#!/bin/bash

function build_file()
{
    echo "building: '$1'";
    if test $1 -nt $1.stamp; then 
        valac -C --pkg gtk+-2.0 --library=${1/.vala/} --pkg cairo --pkg config --pkg gmpc --pkg libmpd --pkg gmpc-rating --pkg gmpc-favorites --pkg gmpc-plugin --pkg gmpc-song-links --pkg gmpc-mpddata-treeview-tooltip  --pkg gmpc-paned-size-group --pkg=gmodule-2.0 --vapidir=`pwd` $2 $1 -H ${1/vala/h} && touch $1.stamp
    fi;

}


valac -C gmpc-paned-size-group.vala  --library=gmpc-paned-size-group --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd` -H gmpc-paned-size-group.h 

valac -C --pkg gtk+-2.0 --library=gmpc-rating --pkg cairo --pkg config --pkg gmpc --pkg libmpd  --pkg gmpc-paned-size-group --pkg gmpc-plugin --vapidir=`pwd` gmpc-rating.vala -H gmpc-rating.h 

valac -C gmpc-plugin.vala  --library=gmpc-plugin --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-paned-size-group --vapidir=`pwd` -H gmpc-plugin.h 

valac -C --pkg gtk+-2.0 --library=gmpc-favorites --pkg cairo --pkg config --pkg gmpc --pkg libmpd  --pkg gmpc-paned-size-group --pkg gmpc-plugin --vapidir=`pwd` gmpc-favorites.vala -H gmpc-favorites.h 

valac -C --pkg gtk+-2.0 --library=gmpc-song-links --pkg cairo --pkg config --pkg gmpc --pkg libmpd  --pkg gmpc-paned-size-group --pkg gmpc-plugin --vapidir=`pwd` gmpc-song-links.vala -H gmpc-song-links.h 

valac -C --pkg gtk+-2.0 --library=gmpc-mpddata-treeview-tooltip --pkg cairo --pkg config --pkg gmpc --pkg libmpd  --pkg gmpc-paned-size-group --pkg gmpc-plugin --vapidir=`pwd` gmpc-mpddata-treeview-tooltip.vala -H gmpc-mpddata-treeview-tooltip.h 

# build_file "gmpc-song-links.vala"

#-H gmpc-plugin.h

build_file "gmpc-image.vala"
build_file "gmpc-progress.vala"
build_file "gmpc-easy-command.vala"
build_file "gmpc-test-plugin.vala"
build_file "gmpc_menu_item_rating.vala" 
#"gmpc-rating.vala"
build_file "gmpc-liststore-sort.vala"
build_file "gmpc-metadata-prefetcher.vala"

build_file "gmpc-metadata-browser2.vala" 

build_file "gmpc-url-fetching-gui.vala"
#"gmpc-song-links.vala gmpc-favorites.vala gmpc-rating.vala"

valac -C --pkg libmpd --vapidir=`pwd` "gmpc-connection.vala" -H gmpc-connection.h
