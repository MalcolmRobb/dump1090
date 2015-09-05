#!/bin/bash

# This script checks for a new version of BasicAircraftLookup from
# the VRS website and, if one is available, downloads it and updates
# the dump1090 webmap json files.

set -e

# defaults that can be overridden:
VRS_URL=${VRS_URL:-http://www.virtualradarserver.co.uk/Files/BasicAircraftLookup.sqb.gz}
CACHEDIR=${CACHEDIR:-/var/cache/dump1090-mutability}
JSONDIR=${JSONDIR:-$CACHEDIR/db}
SQBDIR=${SQBDIR:-$CACHEDIR/sqb}
LOGFILE=${LOGFILE:-/var/log/dump1090-mutability.log}
UPDATESCRIPT=${UPDATESCRIPT:-/usr/share/dump1090-mutability/vrs-basicaircraft-to-json.py}

if [ -f /etc/default/dump1090-mutability ]
then
  . /etc/default/dump1090-mutability
fi

ETAGFILE=$SQBDIR/BasicAircraftLookup.sqb.etag
SQBFILE=$SQBDIR/BasicAircraftLookup.sqb

CHECKMODIFIED=true
LOGTOFILE=false
while [ "$#" -gt 0 ]
do
  case "$1" in
    -f|--force) CHECKMODIFIED=false ;;
    -l|--log-to-file) LOGTOFILE=true ;;
    *) echo "unrecognized option: $1" >&2; exit 1 ;;
  esac
  shift
done

if $LOGTOFILE; then exec >>$LOGFILE 2>&1; fi

log() {
  date "+%c $*" >&2
}

mkdir -p $CACHEDIR $JSONDIR $SQBDIR
rm -f $ETAGFILE.new $SQBFILE.new

log "Checking VRS server for an updated database.."

# get ETag
curl --silent --fail --include --head $VRS_URL | grep ETag >$ETAGFILE.new

# check for existing file
RETRIEVE=true
ARGS=""
if $CHECKMODIFIED && [ -f $SQBFILE ]
then
  if [ -s $ETAGFILE -a -s $ETAGFILE.new ]
  then
    if cmp -s $ETAGFILE $ETAGFILE.new
    then
      log "Database not modified."
      RETRIEVE=false
    else
      log "Database modified, will retrieve a new copy."
    fi
  else
    # do an if-modified-since
    log "Database possibly modified, will try to retrieve a new copy."
    ARGS="-z $SQBFILE"
  fi
fi

if $RETRIEVE
then
  log "Retrieving database.."
  curl --silent --fail --remote-time --retry 2 $ARGS -o $SQBFILE.new $VRS_URL
  mv $ETAGFILE.new $ETAGFILE
  if [ -f $SQBFILE.new ]
  then
    log "Decompressing database.."
    zcat $SQBFILE.new >$SQBFILE
    touch -r $SQBFILE.new $SQBFILE
    rm $SQBFILE.new
  else
    log "Database not modified."
  fi
fi

UPDATE=true
if $CHECKMODIFIED
then
  if test -f $JSONDIR/last_update; then
    if ! test $SQBFILE -nt $JSONDIR/last_update; then UPDATE=false; fi
  fi
fi

if $UPDATE
then
  log "Updating JSON files from database.."
  mkdir -p $JSONDIR/new
  $UPDATESCRIPT $SQBFILE $JSONDIR/new
  rm -f $JSONDIR/*.json
  mv $JSONDIR/new/*.json $JSONDIR/
  touch -r $SQBFILE $JSONDIR/last_update
  rmdir $JSONDIR/new
  log "Done."
else
  log "No update to JSON files needed."
fi

