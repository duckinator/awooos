#!/usr/bin/python3

from configparser import ConfigParser, ExtendedInterpolation
import inspect
from pathlib import Path
import shlex
from subprocess import check_output
import sys
from types import SimpleNamespace as AttrDict

class StringyList(list):
    def __str__(self, sep=" "):
        return sep.join(map(shlex.quote, self))

def fuck(obj):
    if isinstance(obj, dict):
        return fuck_dict_values(obj)
    else:
        return obj

def fuck_dict_values(obj):
    fuck_value = lambda x: (x[0], fuck2(x[1]))
    return dict(map(fuck_value, obj.items()))

def fuck2(obj):
    if isinstance(obj, list):
        return StringyList(map(fuck2, obj))
    elif isinstance(obj, dict):
        return AttrDict(**fuck_dict_values(obj))
    else:
        return obj

def load_config(config_file):
    config = ConfigParser(interpolation=ExtendedInterpolation())
    with open(config_file) as f:
        config.read_file(f)

    platform = config["DEFAULT"]["build.platform"]
    config["platform"] = config["platform:" + platform]

    return config

class BuildCommands:
    def __init__(self, config):
        self.config = config
        self.commands = {}
        self.aliases  = {}
        self.requires = {}
        self.add_all(self.config)
        self._add_aliases(self.config["aliases"])

    def add_all(self, config):
        for section_name in config.sections():
            section = config[section_name]
            if "command" in section:
                self.add(section_name, section)

    def add(self, section_name, section):
        self.commands[section_name] = section.get("command", None)
        self.requires[section_name] = section.get("require", [])

    def _add_aliases(self, aliases):
        for alias, value in aliases.items():
            self.aliases[alias] = value

    def deps_for(self, target):
        return self.requires[self.resolve_name(target)]

    def category_for(self, name):
        if "." in name:
            category = name.split(".", 1)[0]
        elif ":" in name:
            category = name.split(":", 1)[1]
        else:
            category = name

        if not category in self.config["project"]["categories"]:
            return (None, None)
        return ("categories:{}".format(category), name)

    def artifact_path(self, name, category):
        path = Path("artifacts", name + ".INVALID_SUFFIX")
        return path.with_suffix(category["suffix"])

    def category_command_format(self, category_name, name):
        category = self.config[category_name]
        command = category["command"]
        command = shlex.split(command)
        values = fuck({
            "category": category_name,
            "name": name,
            "artifact": {
                "name": name,
                "file": self.artifact_path(name, category),
            },
            "artifacts": {
                "c":    ["ARTIFACTS_C"],
                "asm":  ["ARTIFACTS_ASM"],
            },
            "category": {
                "libraries": {
                    "artifacts": ["LIBRARY_ARTIFACTS"],
                },
            },
        })
        return [arg.format(**values) for arg in command]

    def category_command(self, name):
        category, name = self.category_for(name)
        if category is None:
            print("error: Unknown category: {}".format(category))
            exit(1)
        return self.category_command_format(category, name)

    def resolve_name(self, name):
        if name in self.aliases:
            return self.resolve_name(self.aliases[name])
        elif name in self.commands:
            return self.commands[name]
        elif self.category_for(name)[0]:
            return self.category_for(name)[0]
        else:
            return None

    def __getitem__(self, name):
        resolved_name = self.resolve_name(name)

        if self.category_for(resolved_name):
            return self.category_command(resolved_name)
        elif resolved_name in self.commands:
            return self.commands[resolved_name]
        else:
            print("error: No such target: {}".format(name))
            exit(1)

class Builder:
    def __init__(self, config_file):
        self.config = load_config(config_file)
        self.commands = BuildCommands(self.config)

    def build_deps(self, target):
        deps = self.commands.deps_for(target)

    def build_all(self, targets):
        return [self.build(target) for target in targets]

    def build(self, target):
        command = self.commands[target]
        self.build_deps(target)
        print("> {}".format(target))
        print("$ {}".format(" ".join(command)))

def _main(argv = None):
    """
    TODO: Documentation.
    """
    argv = argv or sys.argv
    args = argv[1:]
    if "--help" in args or "-h" in args:
        print(inspect.getdoc(_main))
        return 1

    targets = set(args)
    if len(targets) == 0:
        targets.update(["all"])

    print("Targets:")
    for target in targets:
        print("- {}".format(target))

    builder = Builder("build.cfg")
    builder.build_all(targets)

    return 0


if __name__ == "__main__":
    exit(_main())
