#!/usr/bin/env python3
# -*- coding: utf-8 -*-
## villoc.py for  in /home/wapiflapi
##
## Made by Wannes Rombouts
## Login   <wapiflapi@epitech.net>
##
## Started on  Sat Apr 18 17:04:24 2015 Wannes Rombouts
## Last update Sun Apr 19 23:32:30 2015 Wannes Rombouts
##

import re
import html
import random
import codecs

def roundup(s):

    if s < 0x20:
        s = 0x20

    if (s & 0xf) == 0:
        return s

    return s - (s & 0xf) + 0x10


class State(list):

    def __init__(self, *args, **kwargs):
        self.errors = []
        self.info = []
        super().__init__(*args, **kwargs)

    def boundaries(self):
        bounds = set()
        for block in self:
            lo, hi = block.boundaries()
            bounds.add(lo)
            bounds.add(hi)
        return bounds

class Printable():

    unit_width = 10
    classes = ["block"]

    def boundaries(self):
        return (self.start(), self.end())

    def gen_html(self, out, width, color=""):
        out.write('<div class="%s" style="width: %dem; %s;">' % (
                " ".join(self.classes), 10 * width, color))
        if self.details:
            out.write('<strong>%#x</strong><br />+ %#x ' % (
                    self.start(), self.end() - self.start()))
            out.write(self.more_html())

        else:
            out.write('&nbsp;')


        out.write('</div>\n')

    def more_html(self):
        return ""

class Empty(Printable):

    classes = Printable.classes + ["empty"]

    def __init__(self, start, end, display=True):
        self._start = start
        self._end = end
        self.details = display

    def start(self):
        return self._start

    def end(self):
        return self._end

    def set_end(self, end):
        self._end = end

class Block(Printable):

    classes = Printable.classes + ["normal"]

    def __init__(self, addr, size, **kwargs):
        self.color = kwargs.get('color', random_color())
        self.uaddr = addr
        self.usize = size
        self.details = True
        self.error = False

    def start(self):
        return self.uaddr - 8

    def end(self):
        return self.uaddr - 8 + roundup(self.usize + 8)

    def gen_html(self, out, width):
        color = "background-color: rgb(%d, %d, %d);" % self.color

        if self.error:
            color += "background-image: repeating-linear-gradient(120deg, transparent, transparent 1.40em, #A85860 1.40em, #A85860 2.80em);"

        super().gen_html(out, width, color)

    def more_html(self):
        return "(%#x)" % self.usize


def malloc(state, ret, size):

    if not ret:
        state.errors.append("Failed to allocate %#x bytes." % size)
    else:
        state.append(Block(ret, size))


def match_ptr(state, ptr):

    if ptr is None:
        return None, None

    s, smallest_match = None, None

    for i, block in enumerate(state):
        if block.uaddr == ptr and (smallest_match is None or smallest_match.usize >= block.usize):
            s, smallest_match = i, block

    if smallest_match is None:
        state.errors.append("Can't find block at %#x." % (ptr-8))

    return s, smallest_match


def calloc(state, ret, nmemb, size):
    malloc(state, ret, nmemb * size)


def free(state, ret, ptr):

    if ptr is 0:
        return

    s, match = match_ptr(state, ptr)

    if match is None:
        return
    elif ret is None:
        state[s] = Block(match.uaddr, match.usize, color=match.color)
        state[s].error = True
    else:
        del state[s]


def realloc(state, ret, ptr, size):

    if not ptr:
        return malloc(state, ret, size)
    elif not size:
        return free(state, ret, ptr)

    s, match = match_ptr(state, ptr)

    if match is None:
        return
    elif ret is None:
        state[s] = Block(match.uaddr, match.usize, color=match.color)
        state[s].error = True
    else:
        state[s] = Block(ret, size, color=match.color)


operations = {
    'free': free,
    'malloc': malloc,
    'calloc': calloc,
    'realloc': realloc,
}

def sanitize(x):
    if x is None:
        return None
    if x == "<void>":
        return 0
    return int(x, 0)


def parse_ltrace(ltrace):

    for line in ltrace:

        if not any(line.startswith(f) for f in operations):
            continue

        try:
            func, args, ret = re.findall(r"^([a-z_]+)\((.*)\) += (.*)", line)[0]
        except:

            try:
                # maybe this stoped the program
                func, args = re.findall(r"^([a-z_]+)\((.*) <no return \.\.\.>", line)[0]
                ret = None
            except:
                print("ignoring line: %s" % line, file=sys.stderr)
                continue

        args = args.split(", ")

        args = list(map(sanitize, args))
        ret = sanitize(ret)

        yield func, args, ret


def build_timeline(events, overhead=16):

    timeline = [State()]

    for func, args, ret in events:

        try:
            op = operations[func]
        except KeyError:
            continue

        state = State(timeline[-1])
        timeline.append(state)

        if ret is None:
            state.errors.append("%s(%s) = <error>" % (func, ", ".join("%#x" % a for a in args)))
        else:
            state.info.append("%s(%s) = %#x" % (func, ", ".join("%#x" % a for a in args), ret))

        op(state, ret, *args)


    return timeline

def random_color(r=200, g=200, b=125):

    red = (random.randrange(0, 256) + r) / 2
    green = (random.randrange(0, 256) + g) / 2
    blue = (random.randrange(0, 256) + b) / 2

    return (red, green, blue)


def print_state(out, boundaries, state):

    out.write('<div class="state %s">\n' % ("error" if state.errors else ""))

    known_stops = set()

    todo = state
    while todo:

        out.write('<div class="line" style="">\n')

        done = []

        current = None
        last = 0

        for i, b in enumerate(boundaries):

            if current and current.end() != b:
                continue

            if current:  # stops here.
                known_stops.add(i)
                current.gen_html(out, i - last)
                done.append(current)
                last = i

            current = None
            for block in todo:
                if block.start() == b:
                    current = block
                    break
            else:
                continue

            if last != i:

                # We want to show from previous known_stop.

                for s in reversed(range(last, i+1)):
                    if s in known_stops:
                        break

                if s != last:
                    Empty(boundaries[last], boundaries[s], display=False).gen_html(out, s - last)
                    known_stops.add(s)

                if s != i:
                    Empty(boundaries[s], b).gen_html(out, i - s)
                    known_stops.add(i)

                last = i


        if current:
            raise RuntimeError("block was started but never finished")

        todo = [x for x in todo if x not in done]

        out.write('</div>\n')


    for msg in state.info:
        out.write('<p>%s</p>' % html.escape(str(msg)))

    for msg in state.errors:
        out.write('<p>%s</p>' % html.escape(str(msg)))

    out.write('</div>\n')

def gen_html(timeline, out):

    if timeline and not timeline[0]:
        timeline.pop(0)

    boundaries = set()
    for state in timeline:
        sb = state.boundaries()
        boundaries.update(sb)

    boundaries = list(sorted(boundaries))

    out.write('<style>')

    out.write('''body {
background-color: #EBEBEB;
font-family: "Lucida Console", Monaco, monospace;
width: %dem;
}
''' % ((len(boundaries) - 1) * (Printable.unit_width + 1) ))

    out.write('''p {
margin: 0.8em 0 0 0.1em;
}
''')

    out.write('''.block {
float: left;
padding: 0.5em 0;
text-align: center;
color: black;
}
''')

    out.write('''.normal {

-webkit-box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);
-moz-box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);
box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);

}
''')

    out.write('''.empty + .empty {
border-left: 1px solid gray;
margin-left: -1px;
}
''')

    out.write('''.empty {
color: gray;
}
''')

    out.write('.line {  }')

    out.write('''.line:after {
  content:"";
  display:table;
  clear:both;
}''')


    out.write('''.state {
margin: 0.5em; padding: 0;

background-color: white;

border-radius: 0.3em;

-webkit-box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);
-moz-box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);
box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);

padding: 0.5em;

}''')


    out.write('''.error {
color: white;
background-color: #8b1820;
}''')

    out.write('''.error .empty {
color: white;
}''')


    out.write('</style>\n')

    out.write('<body>\n')

    out.write('<div class="timeline">\n')


    for state in timeline:
        print_state(out, boundaries, state)


    out.write('</div>\n')

    out.write('</body>\n')



if __name__ == '__main__':

    import sys
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("ltrace", type=argparse.FileType("r"), default=sys.stdin)
    parser.add_argument("out", type=argparse.FileType("w"))
    parser.add_argument("-o", "--overhead", type=int, default=16)
    parser.add_argument("-s", "--seed", type=int, default=226) # 38, 917, 190, 226
    parser.add_argument("-S", "--show-seed", action="store_true")
    args = parser.parse_args()

    random.seed(args.seed)

    if args.show_seed:
        args.out.write('<h2>seed: %d</h2>' % args.seed)

    nice_input = codecs.getreader('utf8')(args.ltrace.detach(), errors='ignore')
    timeline = build_timeline(parse_ltrace(nice_input), overhead=8)

    # for state in timeline:
    #     print(", ".join("%#x: %#x" % (k, v) for k, (v, c) in state.items()))

    gen_html(timeline, args.out)
