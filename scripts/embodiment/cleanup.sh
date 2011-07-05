#!/bin/bash
# remove log and pet database directories
rm -rf /tmp/$USER/opencog/

# the -p of these commands prevent them from complaining if the directories
# already exist
mkdir -p /tmp/$USER/opencog/log
mkdir -p /tmp/$USER/opencog/agent_db

