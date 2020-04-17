
from .parser import Parser
from .tokenizer import tokenize

def parse_expression(exp):
    if not exp.endswith(';'):
        exp = exp + ';'

    tokens = tokenize(exp)
    parser = Parser(tokens)

    return parser.parse_expression()

def parse_member_signature(sig):
    if not sig.endswith(';'):
        sig = sig + ';'

    tokens = tokenize(sig)
    parser = Parser(tokens)

    return parser.parse_member_declaration()

def parse_constructor_signature(sig):
    # Add an empty body to the signature, replacing a ; if necessary
    if sig.endswith(';'):
        sig = sig[:-1]
    sig = sig + '{ }'

    tokens = tokenize(sig)
    parser = Parser(tokens)

    return parser.parse_member_declaration()

def parse_type(s):
    tokens = tokenize(s)
    parser = Parser(tokens)

    return parser.parse_type()

def parse_type_signature(sig):
    if sig.endswith(';'):
        sig = sig[:-1]
    sig = sig + '{ }'

    tokens = tokenize(sig)
    parser = Parser(tokens)

    return parser.parse_class_or_interface_declaration()

def parse(s):
    tokens = tokenize(s)
    parser = Parser(tokens)
    return parser.parse()
