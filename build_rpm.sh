#!/bin/sh
rpmbuild  --define "_topdir `pwd`/rpmbuild"  --define "flrldir `pwd`" --target $1 -bb flrexuizlauncher.spec
