# LUCI Builder Migration - FAQ

LUCI which stands for Layered Universal Continuous Integration is a replacement
for Buildbot (our existing single-threaded monolithic continuous integration
system). Over time, we have been swapping out Buildbot responsibilities with
LUCI services. As such, most builds today on Buildbot are already running using
LUCI services. The next step is to migrate Buildbot builders to LUCI builders,
with the end result to turndown Buildbot.

As a Chromium developer, **read this** to understand the CQ and CI
user expectations and how to contact us.

[TOC]

## FAQ

### **Why can't we keep using existing builders?**

_The existing Buildbot builders are not running on the LUCI stack, as such, they
are still prone to Buildbot limitations such as single process scheduling and
deployment issues (restarts needed) to name a few. We have been in preparation
to migrate builders to LUCI with as minimal impact as possible to Chromium devs._

### **What is the timeframe of migrating Buildbot builders to LUCI?**

*We have started migration of existing Buildbot builders to LUCI builders in
**Q1 2018** and will first focus on the Chromium CQ builder sets. A Chromium
CQ builder set consists of try-builders that are part of the commit queue and their
corresponding waterfall (continuous integration) ci-builders. A builder set will
be migrated together whenever possible. Target migration time per builder set is
approx. 1 week per set with a few days of overlap with existing builders to
ensure correctness and performance. Note that times can vary per builder set.*

### **Who is involved with the builder migrations?**

*We have created a LUCI migration task force 
([infra-dev@chromium.org](mailto:infra-dev@chromium.org)) comprised of Chrome
Operations developers. Owners of Chromium builders are responsible for approvals
for final go ahead to switch a LUCI builder to production.*

### **Where can I learn more about LUCI?**

*A tour of LUCI is available in [presentation format](http://bit.ly/2kgyE9U).
Also, you can learn more about the [LUCI UI under Chromium
source](tour_of_luci_ui.md).*

### **Will the builder migration affect my work?**

*No, there should be minimal to no disruption. Our migration plan is to make it
as seamless as possible for Chromium devs to continue their work and use our CQ
and CI systems effectively.*

### **What limitations does the migration have?**

*While we're working to make the transition as seamless as possible, there can 
be a few hiccups that may occur when a builder is migrated. Below are the 
known limitations:*

* *The blamelist will not be calculated correctly between the first LUCI build
  and the last Buildbot build. The second and subsequent LUCI builds will have
  correct blamelists, however.*

### **What differences will I see?**

*All Chromium developers should already be using the new LUCI UI
([ci.chromium.org](https://ci.chromium.org)). The views will continue to look the
same once LUCI builders are switched to production. Sheriff-O-Matic,
Gatekeeper, and Findit build integrations will all work as intended. The
main differences to expect are:*

* *CQ try builders will show a LUCI tag in the Code Review UI. See
  [below](#How-do-I-know-a-build-was-run-on-LUCI) for details.*
* *LUCI builder pages will have a different URL path. See [UI
  tour](tour_of_luci_ui.md#builder-page)
  for details.*
* *LUCI build pages will have a different URL path. See
  [UI tour](tour_of_luci_ui.md#build-results-page)
  for details.*
* *When the switch is made, build numbers will be incremented
  by approximately +10 to separate Buildbot and LUCI
  builds.*
* *Blamelist limitation between first LUCI and last
  Buildbot builds. See
  [above](#What-limitations-does-the-migration-have) for
  details.*

### **How do I know a build was run on LUCI?**

*Builds on LUCI use a different URL path that
starts with `/p/chromium/builders/…`
If the build is still running on Buildbot,
"buildbot" will still be part of the URL path. Also, in
the Code Review UI, try-builders running on LUCI are
shown with a LUCI tag as shown below.*

![LUCI Code Review UI Tag](images/LUCI-CodeReview-Tag.png
"linux%5chromium%5rel%5ng is a LUCI builder")

*In some cases, builds on LUCI can also have the
following URL
`ci.chromium.org/p/chromium/builds/b<buildbucket_build_id>`.
This occurs when the build does not have a build number 
which should not occur for any Chromium build*

### **When will Buildbot be turned down?**

_Buildbot builders will be turned down progressively and won't be turned down
all at once. Once a CQ builder set has completed migration, we will schedule a
date within the following month to turn down the corresponding Buildbot
builders. This date will be coordinated between the development teams and Chrome
Operations._

### **How do I escalate for help?**

_Contact the Chrome Operations Foundation team
([infra-dev@chromium.org](mailto:infra-dev@chromium.org)) and include
[dpranke@chromium.org](mailto:dpranke@chromium.org),
[estaab@chromium.org](mailto:estaab@chromium.org),
[efoo@chromium.org](mailto:efoo@chromium.org) and
[nodir@chromium.org](mailto:nodir@chromium.org) directly for an immediate
response. Also, refer to the [Contact us](#Contact-Us) section below for other
options to reach us._

### **What are the steps involved to migrate builders to LUCI bots?**

_Builder migration involves 3 major steps._
1. _Preparation (machine allocation/configs)._
2. _Making builds work on LUCI (recipes)._
3. _Flipping to LUCI builder on prod (UI)._

### **Can I help?**

_There are many ways you can help. Either through contributing to LUCI, helping
with builder migrations or providing feedback. Reach out to the Chrome
Operations Foundation team
([infra-dev@chromium.org](mailto:infra-dev@chromium.org)) and CC
[efoo@chromium.org](mailto:efoo@chromium.org) and
[estaab@chromium.org](mailto:estaab@chromium.org) to let us know._

## **Contact Us**

_We are continually making improvements to the migration process and to LUCI
services. Our initial goal is to migrate all Chromium CQ builders sets first as
seamless as possible for Chromium devs to continue to work unimpacted and use
our CQ and CI systems effectively._

_If you have issues or feedback you would like to share with us, we would love to
hear it and incorporate it into our ongoing LUCI improvements._

* _Use the __feedback button__ ![LUCI Feedback](images/LUCI-Feedback-Icon.png
"Feedback")
on a LUCI page._
* *__File a migration bug__ using the following
[template](https://bugs.chromium.org/p/chromium/issues/entry?labels=LUCI-Backlog,LUCI-Migrations&summary=[LUCI-Migration-Bug]%20Enter%20an%20one-line%20summary&components=Infra>Platform&cc=efoo@chromium.org,estaab@chromium.org,nodir@chromium.org&description=Please%20use%20this%20to%20template%20to%20file%20a%20bug%20into%20LUCI%20backlog.%20%20%0A%0AReminder%20to%20include%20the%20following%3A%0A-%20Description%20of%20issue%0A-%20Priority%0A-%20Is%20this%20a%20blocker...%0A-%20What%20builder%20is%20this%20bug%20blocking).*
* *__File a feature request__ using the following
[template](https://bugs.chromium.org/p/chromium/issues/entry?labels=LUCI-Backlog&summary=[LUCI-Feedback]%20Enter%20an%20one-line%20summary&components=Infra>Platform&cc=efoo@chromium.org,estaab@chromium.org,nodir@chromium.org&description=Please%20use%20this%20to%20template%20to%20file%20a%20feature%20request%20into%20LUCI%20backlog.%20%20%0A%0AReminder%20to%20include%20the%20following%3A%0A-%20Description%0A-%20Why%20this%20feature%20is%20needed).*
* *To __share your feedback__, please fill out this [short
survey](https://goo.gl/forms/YPO6XCQ3q47r00iw2).*
* *__Ask a question__ using [IRC under the #chromium](https://www.chromium.org/developers/irc) channel.*
* *__Contact us__ directly by emailing us at
[infra-dev@chromium.org](mailto:infra-dev@chromium.org).*

