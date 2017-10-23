#!/bin/bash
#
# Update all lwip stack from its respective upstream
#

# only run git stash if the working directory actually has changes; otherwise
# we stash nothing and stash pop old stuff.
WD_HAS_CHANGES=$(git diff-index --quiet HEAD --; echo $?)

# we need a clean working directory for subtree
[ $WD_HAS_CHANGES == 1 ] && git stash

# lwip
git subtree pull -m "Update lwip_demo" --prefix lwip_demo/lwip https://git.savannah.gnu.org/git/lwip.git master --squash

# reapply our changes to the working directory
[ $WD_HAS_CHANGES == 1 ] && git stash pop
