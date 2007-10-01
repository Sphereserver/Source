--- STONE SYSTEM ---

This directory contains the de-harcoded stone system (guild & town).
The filenames pretty explains the content of them, but anyway here some explaination:

- The [stones] directory contains generic scripts that are used by towns and guilds,
  the subdirectoris [guilds] and [towns] contain scripts specifics only to them.
- The files that ends in *_config* contains values (_AND ONLY THE VALUES_) that can be
  modified (unless stated otherwise) to apply a rapid customization to the whole system.
- The files that ends in *_functions* contains functions that are used by the system,
  some of these functions are called directly from sphere executable (they're signaled).
  You can generically modify the content of the functions to customize the system, but
  beware of the original way of working to avoid crappy behaviours.
- The files that contains *_dlg_* are the dialog used through the system.
- The files that ends in *_events* and *_speech* are events and speeches applied, if
  enabled in the config, to the players that joins the stone. You can modify them but
  pay attention while doing it.
- The files that ends in *_promptconsole* contains the prompts used to collect user
  input fro the user when needed.
- The files that ends in *_types* contains types applied to items used in the system.
- The files that ends in *_items* contains items used in the system.