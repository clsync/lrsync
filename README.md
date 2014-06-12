lrsync
======
Contents
--------

0. Name
1. Motivation
2. How does that work
3. Use cases
4. Support


0. Name
-------

"lrsync" means "live [rsync](http://rsync.samba.org/)".


1. Motivation
-------------

To make [clsync](https://github.com/xaionaro/clsync) more convenient and useful.

Well, honestly, this's quite useless utility because you can work with clsync directly :)

2. How does that work
---------------------

"lrsync" just parses arguments and sorts it for clsync and rsync (via clsync's "sync-handler-arguments"), after that it calls clsync this way: `clsync -Klrsync -Mrsyncdirect -S path_to_rsync -W source_directory -D destination_directory -L /tmp/.lrsync clsync_arguments -- %RSYNC-ARGS% rsync_arguments`. In this command `clsync_arguments`, `rsync_arguments`, `source_directory` and `destination_directory` are parsed from lrsync arguments.

You can see the result clsync command using option "--clsync-command-only". For example:

    $ lrsync --exit-on-no-events --max-iterations=20 -av 1/ 2/ --clsync-command-only
    clsync \
      -Klrsync \
      -Mrsyncdirect \
      -S \
      rsync \
      -W \
      1/ \
      -D \
      2/ \
      -L \
      /tmp/.lrsync \
      --exit-on-no-events \
      --max-iterations=20 \
      -- \
      %RSYNC-ARGS% \
      -av \
      1/ \
      2/ 

3. Use cases
------------

1. This may be useful if you have a directory that is actively updated. And you want to move it somewhere. You can just call:

    lrsync --exit-hook /root/apacherestart.sh --exit-on-no-events --max-iterations 20 --inplace -av /usr/www/ /var/www/

2. You want to get two directories in synchronous state:

    lrsync --delay-sync 3 --delay-collect 3 --delay-collect-bigfile 3 -av /home/dir1 /home/dir2

4. Support
----------

Write-to:
* dyokunev@ut.mephi.ru 0x8E30679C
* irc.freenode.net#clsync
