#!/bin/bash
# This script will push changes to both origin and GitHub

# Add all changes
git add .

# Commit changes with a message
git commit -m "$(date '+%Y-%m-%d %H:%M:%S')"

# Push to origin
git p origin master

# Push to GitHub
git push github master
