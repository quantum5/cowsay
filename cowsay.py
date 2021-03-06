from __future__ import print_function

import sys
import os
import io
import re
import string
import argparse
import fileinput

from functools import partial
from textwrap import fill as wrap
from itertools import islice

try:
    from cStringIO import StringIO
except ImportError:
    try:
        from io import StringIO
    except ImportError:
        from StringIO import StringIO

try:
    from itertools import imap as map
except ImportError:
    pass

__version__ = '1.00'


try:
    cowpath = os.environ['COWPATH'].split(os.pathsep)
except KeyError:
    cowpath = []

scriptdir = os.path.dirname(__file__)
for dir in (os.path.expanduser('~/.cows'), os.path.expanduser('~/cows'),
            '../share/cows', '../usr/share/cows',
            '/usr/share/cows', '/usr/local/share/cows',
            'cows'):
    if not os.path.isabs(dir):
        dir = os.path.join(scriptdir, dir)
    if os.path.isdir(dir):
        cowpath.append(dir)
del scriptdir, dir

def findcow(file, path=cowpath):
    if os.path.isabs(file):
        return file
    for dir in path:
        check = os.path.join(dir, file)
        if os.path.isfile(check):
            return check
    if file.endswith('.cow'):
        raise ValueError('Cow exists not: ' + file)
    return findcow(file + '.cow', path)

def loadcow(file, thoughts, eyes, tongue, cowpath=cowpath):
    cow = io.open(findcow(file, cowpath), encoding='utf-8').read().strip()
    # Strip comments
    file = StringIO(cow)
    cow = []
    for line in file:
        if not line.lstrip().startswith('#'):
            cow.append(line)
    file.close()
    cow = ''.join(cow)
    if '$the_cow' in cow: # Perl format, I will try to be compatible
        end = re.compile(r'\$the_cow\s*=\s*<<["\']?(\w+)["\']?;?$', re.M).search(cow)
        if end is None:
            raise ValueError("Can't find a perl cow declaration")
        start = end.end()
        end = end.group(1)
        # I only support heredoc cows, and this finds the end of it
        end = re.compile('^' + re.escape(end) + '$', re.M).search(cow)
        if end is None:
            end = -1 # screw this, the whole cow it is
        else:
            end = end.start()
        cow = cow[start:end].strip('\r\n')
        
        # The curse of compatiblity
        cow = string.Template(cow.replace(r'\$', '$$')).safe_substitute({
            'thoughts': thoughts,
            'eyes': eyes,
            'tongue': tongue,
        })
        return cow.replace(r'\\', '\\').replace(r'\@', '@')
    else:
        # Now, my own cow format, just basic formatting
        cow = cow.strip('\r\n')
        return cow.format(thoughts=thoughts, eyes=eyes, tongue=tongue)

def make_ballon(lines, think=False):
    maxlen = max(map(len, lines)) + 2
    format = '%s %-' + str(maxlen - 2) + 's %s'
    yield ' %s ' % ('_' * maxlen)
    if think:
        for line in lines:
            yield format % ('(', line, ')')
    elif len(lines) < 2:
        yield format % ('<', lines[0] if lines else '', '>')
    else:
        yield format % ('/', lines[0], '\\')
        for line in islice(lines, 1, len(lines) - 1):
            yield format % ('|', line, '|')
        yield format % ('\\', lines[-1], '/')
    yield ' %s ' % ('-' * maxlen)

def main(prog, out=sys.stdout):
    eyes = 'oo'
    tongue = '  '
    
    parser = argparse.ArgumentParser(description='Python reimplementation of the classic cowsay.')
    parser.set_defaults(eyes='oo')
    parser.set_defaults(tongue=None)
    parser.set_defaults(thoughts='o' if 'think' in os.path.basename(__file__) else '\\')
    parser.set_defaults(cowpath=cowpath[::-1])
    parser.add_argument('-b', '--borg',     action='store_const', dest='eyes', const='==')
    parser.add_argument('-d', '--dead',     action='store_const', dest='eyes', const='xx')
    parser.add_argument('-g', '--greedy',   action='store_const', dest='eyes', const='$$')
    parser.add_argument('-p', '--paranoid', action='store_const', dest='eyes', const='@@')
    parser.add_argument('-s', '--stoned',   action='store_const', dest='eyes', const='**')
    parser.add_argument('-t', '--tired',    action='store_const', dest='eyes', const='--')
    parser.add_argument('-w', '--wired',    action='store_const', dest='eyes', const='OO')
    parser.add_argument('-y', '--young',    action='store_const', dest='eyes', const='..')
    parser.add_argument('-e', '--eyes',     action='store', dest='eyes')
    parser.add_argument('-T', '--tongue',   action='store', dest='tongue')
    parser.add_argument('-l', '--list', action='store_true', dest='list',
                        help='displays cow file location')
    parser.add_argument('-f', '--file',     action='store', dest='file',
                        default='default.cow', help='cow file, searches in cowpath. '
                        '.cow is automatically appended')
    parser.add_argument('-E', '--encoding', action='store', dest='encoding',
                        default='utf-8', help='Encoding to use, utf-8 by default')
    parser.add_argument('-W', '--wrap', action='store', type=int, dest='wrap',
                        default=70, help='wraps the cow text, default 70')
    parser.add_argument('--thoughts', action='store', dest='thoughts',
                        help='the method of communication cow uses. '
                        'Default to `o` if invoked as cowthink, otherwise \\')
    parser.add_argument('-c', '--command-line', action='store_true', dest='cmd',
                        help='treat command line as text, not files')
    parser.add_argument('-a', '--add', '--add-cow', '--add-path',
                        '--add-cow-path', action='append', dest='cowpath')
    parser.add_argument('files', metavar='FILES', nargs='*')
    
    args = parser.parse_args()
    if args.tongue is None:
        if args.eyes in ('xx', '**'): # one of the predefined dead faces
            args.tongue = 'U '
        else:
            args.tongue = '  '
    args.eyes = (args.eyes + '  ')[:2]
    args.tongue = (args.tongue + '  ')[:2]

    args.cowpath.reverse()
    if args.list:
        try:
            print(findcow(args.file, args.cowpath))
        except ValueError as e:
            sys.exit(e.message)
        else:
            sys.exit(0)
    try:
        cow = loadcow(args.file, args.thoughts, args.eyes, args.tongue, args.cowpath)
    except ValueError as e:
        sys.exit(e.message)
    
    if args.cmd:
        input = '\n'.join(args.files)
    else:
        input = ''.join(fileinput.input(args.files,
                openhook=partial(io.open, encoding=args.encoding)))
    input = wrap(input, args.wrap, replace_whitespace=False).split('\n')
    for line in make_ballon(input, args.thoughts == 'o'):
        print(line, file=out)
    print(cow, file=out)

if __name__ == '__main__':
    main(sys.argv[0])
