import sys
import os
import io
import re
import string
import argparse
try:
    from cStringIO import StringIO
except ImportError:
    try:
        from io import StringIO
    except ImportError:
        from StringIO import StringIO

__version__ = '1.00'

try:
    cowpath = os.environ['COWPATH']
except KeyError:
    scriptdir = os.path.dirname(__file__)
    for dir in ('../share/cows', '../usr/share/cows',
                '/usr/share/cows', '/usr/local/share/cows',
                'cows'):
        path = os.path.join(scriptdir, dir)
        if os.path.exists(path):
            cowpath = path
    del scriptdir, path, dir

def loadcow(file, thoughts, eyes, tongue):
    cow = io.open(os.path.join(cowpath, file), encoding='utf-8').read().strip()
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

def main(prog):
    eyes = 'oo'
    tongue = '  '
    
    parser = argparse.ArgumentParser(description='Python reimplementation of the classic cowsay')
    parser.set_defaults(eyes='oo')
    parser.set_defaults(tongue=None)
    parser.set_defaults(thoughts='o' if 'think' in os.path.basename(__file__) else '\\')
    parser.add_argument('-b', '--borg',     action='store_const', dest='eyes', const='==')
    parser.add_argument('-d', '--dead',     action='store_const', dest='eyes', const='xx')
    parser.add_argument('-g', '--greedy',   action='store_const', dest='eyes', const='$$')
    parser.add_argument('-p', '--paranoid', action='store_const', dest='eyes', const='@@')
    parser.add_argument('-s', '--stoned',   action='store_const', dest='eyes', const='**')
    parser.add_argument('-t', '--tired',    action='store_const', dest='eyes', const='--')
    parser.add_argument('-w', '--wired',    action='store_const', dest='eyes', const='OO')
    parser.add_argument('-y', '--young',    action='store_const', dest='eyes', const='..')
    parser.add_argument('-e', '--eyes',     action='store', dest='eyes')
    parser.add_argument('-f', '--file',     action='store', dest='file', default='default.cow')
    parser.add_argument('--tongue',         action='store', dest='tongue')
    parser.add_argument('--thoughts',       action='store', dest='thoughts')
    
    args = parser.parse_args()
    if args.tongue is None:
        if args.eyes in ('xx', '**'): # one of the predefined dead faces
            args.tongue = 'U '
        else:
            args.tongue = '  '
    args.eyes = (args.eyes + '  ')[:2]
    args.tongue = (args.tongue + '  ')[:2]

    print loadcow(args.file, args.thoughts, args.eyes, args.tongue)

if __name__ == '__main__':
    main(sys.argv[0])
