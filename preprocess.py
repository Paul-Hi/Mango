import os, sys, re, string


def things():
    things = [
        'class',
        'enum',
        'struct',
    ]
    return things

def prethings():
    prethings = [
        '_ptr',
    ]
    return prethings

def access():
    access = [
        'static',
        'const',
    ]
    return access

def p_and_r():
    p_and_r = [
        '&',
        '*',
    ]
    return p_and_r

def delimiters():
    delimiters = {
        '(' : 'd_lparenthesis',
        ')' : 'd_rparenthesis',
        '[' : 'd_lbracket',
        ']' : 'd_rbracket',
        '{' : 'd_lbrace',
        '}' : 'd_rbrace',
        '=' : 'd_assign',
        ',' : 'd_comma',
        ';' : 'd_semicolon',
        '<' : 'd_less',
        '>' : 'd_greater',
        '//': 'd_comment',
        '/!': 'd_doc',
        '.' : 'd_point',
        '\r': 'd_cr',
        '\n': 'd_lf',
        '\t': 'd_tab',
        ' ' : 'd_space',
    }
    return delimiters

def is_whitespace(character):
    return character in ['', ' ', '\t', '\r', '\n']

def tokenize_line(line):
    tokens = line.split(' ')
    for delimiter in delimiters().keys():
        for i in range(len(tokens)):
            token = tokens[i]
            if token != delimiter and delimiter in token:
                pos = token.find(delimiter)
                tokens[i] = ' '
                before = token[:pos]
                after = token[pos + 1 :]
                tokens.append(before)
                tokens.append(delimiter)
                tokens.append(after)
            else:
                tokens[i] = ' '
                tokens.append(token)

        tokens = [t for t in tokens if not is_whitespace(t)]

    return tokens

def tokenize_meta(path):
    meta = False
    start_multi_line_c = False
    multi_line_c = False
    tokens = []
    with open(path, 'r') as file:
        for line_count, line in enumerate(file):
            if 'meta(reflect)' in line:
                print('// Reflection in', path, ':', line_count)
                meta = True
            if meta:
                line_tokens = tokenize_line(line)
                start_idx = 0
                end_idx = len(line_tokens)
                if '//' in line_tokens:
                    end_idx = line_tokens.index('//')
                if '/*' in line_tokens:
                    end_idx = line_tokens.index('/*')
                    start_multi_line_c = True
                if '*/' in line_tokens:
                    start_idx = line_tokens.index('*/') + 1
                    multi_line_c = False
                if not multi_line_c:
                    tokens.extend(line_tokens[start_idx:end_idx])
                if start_multi_line_c:
                    multi_line_c = True
                    start_multi_line_c = False
            if '}' in line:
                meta = False
    return tokens

def generate_meta(tokens):
    _delimiters = delimiters().keys()
    _things = things()
    _prethings = prethings()
    _p_and_r = p_and_r()
    _access = access()
    name = False
    field_definition = False
    member = [None] * 3
    for token in tokens:
        if token in _things:
            # thing
            field_definition = True
        elif any(p in token for p in _p_and_r) and not name:
            # prething
            member[0] = token[-1:]
            member[1] = token[:-1]
            name = True
            continue
        elif any(p in token for p in _prethings) and not name:
            # prething
            member[0] = token
            continue
        elif token not in _delimiters:
            # name
            if field_definition:
                print('field_definition ', token, '_fields[] = \r\n{', sep = '')
                field_definition = False
                continue

            idx = 1
            if name:
                idx = 2
            elif '::' in token:
                pos = token.find('::')
                token = token[pos + 2:] # remove namespaces
            name = not name
            member[idx] = token
        else:
            if token == '}':
                print(token, end = ';\r\n')
                return
            if token == ';':
                print('    {', end = ' ')
                if member[0] != None:
                    print('pointer_type', end = ', ')
                else:
                    print('basic_type', end = ', ')
                if member[1] != None:
                    print('type_', member[1], sep = '', end = ', ')
                if member[2] != None:
                    print('"', member[2], '"', sep = '', end = ' ')
                print('},')
                member = [None] * 3


def main(argv):
    print("Creating reflection data.")
    paths = ["mango/include/mango/test.hpp"]
    for path in paths:
        tokens = tokenize_meta(path)[4:]
        generate_meta(tokens)


if __name__ == "__main__":
    main(sys.argv[1:])

