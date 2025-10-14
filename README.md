## About

Uses the [ragel state machine](https://github.com/adrian-thurston/ragel) to generate the dtext parsing capabilities of e621.net

## Getting started

Most of the changes will only need to touch `dtext.rl`, the rest of the files will be generated for you by running either `rake compile` or `rake test`. Take a look at [this unofficial quickstart guide](https://github.com/calio/ragel-cheat-sheet) or the [complete official documentation](http://www.colm.net/files/ragel/ragel-guide-6.10.pdf) if you want to know more about how ragel works.

There's a `docker-compose.yml` which you can use to quickly run the most common commands without installing everything locally. Usable like this: `docker-compose run --rm rake test`. You will need to run `docker-compose build` once beforehand.

## Releasing a new version for usage in e621

Commit the changes to `dtext.cpp.rl` and the resuling changes in `dtext.cpp`. Bump the version number in `lib/dtext/version.rb`. After that is all done you can `bundle lock` in the e6 repository. It should pick up on the increased version.

To test these changes locally commit them and update the `Gemfile`s dtext entry. Specifying the commit hash allows you to rebuild the container without having to also increment the version number every time. Don't forget to `bundle lock` before rebuilding.  
`gem "dtext_rb", git: "https://github.com/YOUR_FORK/dtext_rb.git", ref: "YOUR_COMMIT_HASH"`
