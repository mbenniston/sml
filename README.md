# sml: Simple markup language

Subset of xml which is even easier to parse.

## Features:
- Tags
- Singleton tags
- Attributes
- Content

For an example of an sml file look within the examples folder: [Examples](./examples).

## Things to note

- Whitespace and newlines are stripped from the start and end of content within tags.
- Tags can either contain other tags or content but not both. Content will be ignored if a tag is found within a tag. 
- Comments are not supported.

## In this repo: sml parser

This repo contains an implementation of an sml parser and writer. For usage guide go to [Usage](./usage.md)
