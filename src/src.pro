TEMPLATE = subdirs
SUBDIRS = lib plugin
lib.target = lib-target
plugin.depends = lib-target
