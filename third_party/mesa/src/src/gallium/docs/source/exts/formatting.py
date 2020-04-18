# formatting.py
# Sphinx extension providing formatting for Gallium-specific data
# (c) Corbin Simpson 2010
# Public domain to the extent permitted; contact author for special licensing

import docutils.nodes
import sphinx.addnodes

def parse_envvar(env, sig, signode):
    envvar, t, default = sig.split(" ", 2)
    envvar = envvar.strip().upper()
    t = " Type: %s" % t.strip(" <>").lower()
    default = " Default: %s" % default.strip(" ()")
    signode += sphinx.addnodes.desc_name(envvar, envvar)
    signode += sphinx.addnodes.desc_type(t, t)
    signode += sphinx.addnodes.desc_annotation(default, default)
    return envvar

def parse_opcode(env, sig, signode):
    opcode, desc = sig.split("-", 1)
    opcode = opcode.strip().upper()
    desc = " (%s)" % desc.strip()
    signode += sphinx.addnodes.desc_name(opcode, opcode)
    signode += sphinx.addnodes.desc_annotation(desc, desc)
    return opcode

def setup(app):
    app.add_description_unit("envvar", "envvar", "%s (environment variable)",
        parse_envvar)
    app.add_description_unit("opcode", "opcode", "%s (TGSI opcode)",
        parse_opcode)
