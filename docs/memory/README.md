# Memory

Landing page for all things memory-related in Chromium.

The goal is to keep an up-to-date set of links and references that will
help people understand what is actively happening in the memory space. Please keep
this landing page short and action oriented.

That being said, please also send CL with update and changes. This should
reflect current active status, and it's easier to do that if everyone helps
maintain it. :)

## How is chrome's memory usage doing in the world?

Look at the UMAs **Memory.{Total,Renderer,Browser,Gpu,Extension}.PrivateMemoryFootprint**.

These metrics are known to lack sufficient context. e.g. How many renderers are open? What site is a renderer hosting? How long has the browser been running?

Some of these graphs are now available at [go/mem-ukm](http://go/mem-ukm).


## How do developers communicate?

Note, these channels are for developer coordination and NOT user support. If
you are a Chromium user experiencing a memory related problem, file a bug
instead.

| name | description |
|------|-------------|
| [memory-dev@chromium.org]() | Discussion group for all things memory related. Post docs, discuss bugs, etc., here. |
| chrome-memory@google.com | Google internal version of the above. Use sparingly. |
| https://chromiumdev.slack.com/messages/memory/ | Slack channel for real-time discussion with memory devs. Lots of C++ sadness too. |
| crbug [Performance=Memory](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Performance%3DMemory) label | Bucket with auto-filed and user-filed bugs. |
| crbug [Stability=Memory](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Stability%3DMemory) label | Tracks mostly OOM crashes. |


## I have memory problem, what do I do?

Follow [these instructions](/docs/memory/filing_memory_bugs.md) to file a high
quality bug.

## I'm a developer trying to investigate a memory issues, what do I do?

See [this page](/docs/memory/debugging_memory_issues.md) for further instructions.

## I'm a developer looking for more information. How do I get started?

Great! First, sign up for the mailing lists above and check out the slack channel.

Second, familiarize yourself with the following:

| Topic | Description |
|-------|-------------|
| [Key Concepts in Chrome Memory](/docs/memory/key_concepts.md) | Primer for memory terminology in Chrome. |
| [memory-infra](/docs/memory-infra/README.md) | The primary tool used for inspecting allocations. |

## What are people actively working on?
| Project | Description |
|---------|-------------|
|  [Purge+Throttle/Suspend](https://docs.google.com/document/d/1EgLimgxWK5DGhptnNVbEGSvVn6Q609ZJaBkLjEPRJvI/edit) | Centralized policy and coordination of all memory components in Chrome |
| [Memory-Infra](/docs/memory-infra/README.md) | Tooling and infrastructure for Memory |
| [System health benchmarks](https://docs.google.com/document/d/1pEeCnkbtrbsK3uuPA-ftbg4kzM4Bk7a2A9rhRYklmF8/edit?usp=sharing) | Automated tests based on telemetry |
| [Out of Process Heap Profiling](https://docs.google.com/document/d/1zKNGByeouYz9E719J8dmKNepLLanCNUq310gbzjiHOg/edit#heading=h.aabxwucn5hhp) | Collect heap dumps from the wild |
| [Always on Document Leak Detector](https://bugs.chromium.org/p/chromium/issues/detail?id=757374) | UMA-based sanity check that DOM objects are not leaking in the wild. |
| [Real-world leak detector](https://bugs.chromium.org/p/chromium/issues/detail?id=763280) | Runs blink leak detector on top-sites [web-page-replay] on waterfall.


## Key knowledge areas and contacts
| Knowledge Area | Contact points |
|----------------|----------------|
| Chrome on Android | mariakhomenko, dskiba, ssid |
| Browser Process | mariakhomenko, dskiba, ssid |
| GPU/cc | ericrk |
| Memory metrics | erikchen, primano, ajwong, wez |
| Native Heap Profiling | primiano, dskiba, ajwong |
| Net Stack | mmenke, rsleevi, xunjieli |
| Renderer Process | haraken, tasak, hajimehoshi, keishi, hiroshige |
| V8 | hpayer, ulan, verwaest, mlippautz |
| Out of Process Heap Profiling | erikchen, ajwong, brettw, etienneb


## Other docs
  * [Why we work on memory](https://docs.google.com/document/d/1jhERqimO-LtuplzQzbBv1vK7SVOh63AMf2irJI2LOqU/edit)
  * [TOK/LON memory-dev@ meeting notes](https://docs.google.com/document/d/1tCTw9lnjs85t8GFiiyae2hbu6lrz8kysFCgMCKUvcXo/edit)
  * [Memory convergence 3 (in Mountain View, 2017 May)](https://docs.google.com/document/d/1FBIqBGIa0DSaFsh-QjmVvoC82pGuOgiQDIhc8-vzXbQ/edit)
  * [TRIM convergence 2 (in Mountain View, 2016 Nov)](https://docs.google.com/document/d/17Kef7UxjR6VW_ehVbsc-DI0IU7TQk-2C56JSbzbPuhA/edit)
  * [TRIM convergence 1 (in Munich, 2016 Apr)](https://docs.google.com/document/d/1PGcM6iVBp0OYh3m8xGQhOgkQK0obQy8YWwoefP9NZCA/edit#)

