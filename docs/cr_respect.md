# Respectful Code Reviews
## A Guide for Code Reviewers

_For the code author counterpart, see
__[Respectful Changes](cl_respect.md)__._

## Do

#### Assume competence & goodwill

We attract competent people - and that means even when they're wrong, it most likely comes from lack of information, not from inability. A "bad" CL usually means one of the parties is in possession of information the other one isn't aware of.

#### Discuss in person

If there is a disagreement, have a quick in-person/video/IM chat to sort out what is going on - it's much easier to address all the little "Oh, I didn't know"s in a single face-to-face, instead of back-and-forth via e-mail with long delays. "In person" doubly applies if you are disagreeing with another reviewer. And please make sure to record the outcomes on the review.

#### Explain why

It might be obvious to you that some code is wrong, but it's probably not obvious to the author — or they wouldn't have written it that way. So please don't say "This is wrong". Instead, explain at least what the right way looks like. Or even better, explain *why* they should do things differently. And if you're the slightest bit uncertain, "Maybe I'm missing something, but…" is a helpful sentence. Remember, assume competence.

#### Ask for the why

If it is unclear why the author is doing things a certain way, feel free to ask why they made a particular change. Not knowing is OK, and asking "Why" leaves a written record that will help answer this question in the future. (And sometimes, "I'm curious, why did you decide to do it that way?" can help the author to rethink their decision.)

#### Find an end

If you like things neat, it's tempting to go over a code review over and over until it's perfect, dragging it out for longer than necessary. It's soul-deadening for the recipient, though. Keep in mind that "LGTM" does not mean "I vouch my immortal soul this will never fail", but "looks good to me". If it looks good, move on. (That doesn't mean you shouldn't be thorough. It's a judgment call.) And if there are bigger refactorings to be done, move them to a new CL.

#### Reply within a reasonable timeframe

Please don't leave the reviewee waiting for a long time, keeping in mind timezones and different working hours. If you cannot get to a review within ~24h, please leave a short comment on the CL saying so (and when you can). And if you missed that window, please be courteous if the reviewee IMs you a reminder.

If you will be on vacation or otherwise OOO for more than a few days, please set your nickname in the Chromium code review tool to indicate this (e.g. adding '(OOO until <date>)'). Remember that not everyone sending you code reviews can see your calendar!

#### Mention the positives

It's very easy to get into the mindset of "find ALL the flaws", but acknowledging the positives both helps keep things civil, and brightens the recipient's day. No need to be all fake smiles, but if there's a good decision, or if somebody takes on a really grungy task, acknowledging that is a nice thing to do. And on the converse, a "thank you" to the reviewers is occasionally a nice thing, too.

## Don't

#### Don't shame people

"How could you not see this" is a very unhelpful thing to say. Assume that your colleagues do their best, but occasionally make mistakes. That's why we have code reviews - to spot those mistakes. While flawless CLs are awesome, flawed ones are the norm.

#### Don't use extreme or very negative language

Please don't say things like "no sane person would ever do this" or "this algorithm is terrible", whether it's about the change you're reviewing or about the surrounding code. While it might intimidate the reviewee into doing what you want, it's not helpful in the long run - they will feel incapable, and there is not much info in there to help them improve. "This is a good start, but it could use some work" or "This needs some cleanup" are nicer ways of saying it. Discuss the code, not the person.

#### Don't discourage tool use

If people use the automated formatter, be grateful they're willing to give up the power of formatting in favor of ensuring a consistent code base. Think carefully before enforcing your own preferences over it. If people use the try bots to find bugs in minor changes, don't discourage them - be grateful they're trading machine time to make more room to solve more problems.

#### Don't bikeshed

Always ask yourself if this decision *really* matters in the long run, or if you're enforcing a subjective preference. It feels good to be right, but only one of the two participants can win that game. If it's not important, agree to disagree, and move on.
