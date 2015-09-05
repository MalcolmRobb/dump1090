#!/usr/bin/env python2

#
# Converts a Virtual Radar Server BasicAircraftLookup.sqb database
# into a bunch of json files suitable for use by the webmap
#

import sqlite3, json, sys
from contextlib import closing

def extract(dbfile, todir, blocklimit, debug):
    ac_count = 0
    block_count = 0

    blocks = {}
    for i in xrange(16):
        blocks['%01X' % i] = {}

    print >>sys.stderr, 'Reading', dbfile
    with closing(sqlite3.connect(dbfile)) as db:
        with closing(db.execute('SELECT a.Icao, a.Registration, m.Icao FROM Aircraft a, Model m WHERE a.ModelID = m.ModelID')) as c:
            for icao24, reg, icaotype in c:
                bkey = icao24[0:1].upper()
                dkey = icao24[1:].upper()
                blocks[bkey][dkey] = {}
                if reg: blocks[bkey][dkey]['r'] = reg
                if icaotype: blocks[bkey][dkey]['t'] = icaotype
                ac_count += 1
    print >>sys.stderr, 'Read', ac_count, 'aircraft'

    print >>sys.stderr, 'Writing blocks:',

    queue = sorted(blocks.keys())
    while queue:
        bkey = queue[0]
        del queue[0]

        blockdata = blocks[bkey]
        if len(blockdata) > blocklimit:
            if debug: print >>sys.stderr, 'Splitting block', bkey, 'with', len(blockdata), 'entries..',

            # split all children out
            children = {}
            for dkey in blockdata.keys():
                new_bkey = bkey + dkey[0]
                new_dkey = dkey[1:]

                if new_bkey not in children: children[new_bkey] = {}
                children[new_bkey][new_dkey] = blockdata[dkey]

            # look for small children we can retain in the parent, to
            # reduce the total number of files needed. This reduces the
            # number of blocks needed from 150 to 61
            blockdata = {}
            children = sorted(children.items(), key=lambda x: len(x[1]))
            retained = 1

            while len(children[0][1]) + retained < blocklimit:
                # move this child back to the parent
                c_bkey, c_entries = children[0]
                for c_dkey, entry in c_entries.items():
                    blockdata[c_bkey[-1] + c_dkey] = entry
                    retained += 1
                del children[0]

            if debug: print >>sys.stderr, len(children), 'children created,', len(blockdata), 'entries retained in parent'
            children = sorted(children, key=lambda x: x[0])
            blockdata['children'] = [x[0] for x in children]
            blocks[bkey] = blockdata
            for c_bkey, c_entries in children:
                blocks[c_bkey] = c_entries
                queue.append(c_bkey)

        path = todir + '/' + bkey + '.json'
        if debug: print >>sys.stderr, 'Writing', len(blockdata), 'entries to', path
        else: print >>sys.stderr, bkey,
        block_count += 1
        with closing(open(path, 'w')) as f:
            json.dump(obj=blockdata, fp=f, check_circular=False, separators=(',',':'), sort_keys=True)

    print >>sys.stderr, 'done.'
    print >>sys.stderr, 'Wrote', block_count, 'blocks'

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >>sys.stderr, 'Syntax: %s <path to BasicAircraftLookup.sqb> <path to DB dir>' % sys.argv[0]
        sys.exit(1)
    else:
        extract(sys.argv[1], sys.argv[2], 1000, False)
        sys.exit(0)
