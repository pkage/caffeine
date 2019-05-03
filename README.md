# Caffeine daemon

This is a really simple [caffeine](https://en.wikipedia.org/Caffeine) daemon for your linux system.

This works entirely without requiring root access and bypasses any imposed system management by emulating a mouse wiggle every few minutes.

## Get started

Build the daemon & controller:

```
$ make
```

This produces a `caffeine` artifact in the directory. You can then copy this onto your `$PATH`.

## Usage

Command               | Effect
---                   | ---
`caffeine daemon`     | Start the caffeine daemon. Creates a lockfile in the caffeine config dir.
`caffeine query`      | Check if the daemon is running, and if it is, check if it is keeping the screen awake. Returns a non-zero exit code if not.
`caffeine start`      | Start keeping the screen awake.
`caffeine stop`       | Stop keeping the screen awake.
`caffeine toggle`     | Toggle keeping the screen awake.
`caffeine diagnostic` | Show some diagnostic information (daemon status, etc.)
`caffeine version`    | Show version.

## Configuration

Check out the `config.h` file for configuration options.
