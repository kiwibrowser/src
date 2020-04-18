# Properties of a Good Top Level Metric

When defining a top level metric, there are several desirable properties which are frequently in tension. This document attempts to roughly outline the desirable properties we should keep in mind when defining a metric. Also see the document on improving the actionability of a top level metric via [diagnostic metrics](diagnostic_metrics.md).

[TOC]

## Representative

Top level metrics are how we understand our product’s high level behavior, and if they don’t correlate with user experience, our understanding is misaligned with the real world. However, measuring representativeness is costly. In the long term, we can use ablation studies (in the browser or in partnership with representative sites), or user studies to confirm representativeness. In the short term, we use our intuition in defining the metric, and carefully measure the metric implementation’s accuracy.

These metrics would ideally also correlate strongly with business value, making it easy to motivate site owners to optimizing these metrics.

## Accurate

When we first come up with a metric, we have a concept in mind of what the metric is trying to measure. The accuracy of a metric implementation is how closely the metric implementation aligns to our conceptual model of what we’re trying to measure.

For example, First Contentful Paint was created to measure the first time we paint something the user might actually care about. Our current implementation looks at when the browser first painted any text, image, non-white canvas or SVG. The accuracy of this metric is determined by how often the first thing painted which the user cares about is text, image, canvas or SVG.

To evaluate how accurate a metric is, there’s no substitute for manual evaluation. Ideally, this evaluation phase would be performed by multiple people, with little knowledge of the metric in question.

To initially evaluate the accuracy of a point in time metric:

* Gather a bunch of samples of pages where we can compute our metric.
* Get a group of people unfamiliar with proposed metric implementations to identify what they believe the correct point in time for each sample.
* Measure the variability of the hand picked points in time. If this amount of variability is deemed too high, we’ll need to come up with a more specific metric, which is easier to hand evaluate.
* Measure the error between the implementation results and the hand picked results. Ideally, our error measurement would be more forgiving in cases where humans were unsure of the correct point in time. We don’t have a concrete plan here yet.

To initially evaluate accuracy of a quality of experience metric, we rely heavily on human intuition:

* Gather a bunch of samples of pages where we can compute our metric.
* Get a group of people unfamiliar with the proposed metric implementations to sort the samples by their estimated quality of experience.
* Use [Spearman's rank-order correlation](https://statistics.laerd.com/statistical-guides/spearmans-rank-order-correlation-statistical-guide.php) to examine how well correlated the different orderings are. If they aren’t deemed consistent enough, we’ll need to come up with a more specific metric, which is easier to hand evaluate.
* Use the metric implementation to sort the samples.
* Use [Spearman's rank-order correlation](https://statistics.laerd.com/statistical-guides/spearmans-rank-order-correlation-statistical-guide.php) to evaluate how similar the metric implementation is to the hand ordering.

## Stable

A metric is stable if the result doesn’t vary much between successive runs on similar input. This can be quantitatively evaluated, ideally using Chrome Trace Processor and cluster telemetry on the top 10k sites. Eventually we hope to have a concrete threshold for a specific spread metric here, but for now, we gather the stability data, and analyze it by hand.

Different domains have different amounts of inherent instability - for example, when measuring page load performance using a real network, the network injects significant variability. We can’t avoid this, but we can try to implement metrics which minimize instability, and don’t exaggerate the instability inherent in the system.

## Interpretable

A metric is interpretable if the numbers it produces are easy to understand, especially for individuals without strong domain knowledge. For example, point-in-time metrics tend to be easy to explain, even if their implementations are complicated (see "Simplicity"). For example, it’s easy to communicate what First Meaningful Paint is, even if how we compute it is very complicated. Conversely, something like [SpeedIndex](https://sites.google.com/a/webpagetest.org/docs/using-webpagetest/metrics/speed-index) is somewhat difficult to explain and [hard to reason about](https://docs.google.com/document/d/14K3HTKN7tyROlYQhSiFP89TT-Ddg2aId9uyEsWj5UAY/edit) - it’s the average time at which things were displayed on the page.

Metrics which are easy to interpret are often easier to evaluate. For example, First Meaningful Paint can be evaluated by comparing hand picked first meaningful paint times to the results of a given approach for computing first meaningful paint. SpeedIndex is more complicated to evaluate - we’d need to use the approach given [above](#Accurate) for quality of experience metrics.

## Simple

A metric is simple if the way its computed is easy to understand. There’s a strong correlation between being simple and being interpretable, but there are counter examples, such as FMP being interpretable, but not simple.

A simple metric is less likely to have been overfit during the metric development / evaluation phase, and has other obvious advantages (easier to maintain, often faster to execute, less likely to contain bugs).

One crude way of quantifying simplicity is to measure the number of tunable parameters. For example, we can look at two ways of aggregating Frame Throughput. We could look at the average Frame Throughput defined over all animations during the pageview. Alternatively, we could look for the 300ms window with the worst average Frame Throughput. The second approach has one additional parameter, and is thus strictly more complex.

## Elastic

A good metric is [elastic](https://en.wikipedia.org/wiki/Elasticity_of_a_function), that is, a small change in the input (the page) results in a small change in the output.

In a continuous integration environment, you want to know whether or not a given code change resulted in metric improvements or regressions. Non-elastic metrics often obscure changes, making it hard to justify small but meaningful improvements, or allowing small but meaningful regressions to slip by. Elastic metrics also generally have lower variability.

This is frequently at odds with the interpretability requirement. For example, First Meaningful Paint is easier to interpret than SpeedIndex, but is non-elastic.

If your metric involves thresholds (such as the 50ms task length threshold in TTI), or heuristics (looking at the largest jump in the number of layout objects in FMP), it’s likely to be non-elastic.

## Realtime

We’d like to have metrics which we can compute in realtime. For example, if we’re measuring First Meaningful Paint, we’d like to know when First Meaningful Paint occurred *at the time it occurred*. This isn’t always attainable, but when possible, it avoids some classes of [survivorship bias](https://en.wikipedia.org/wiki/Survivorship_bias), which makes metrics easier to analyze.

# Example

[Time to Consistently Interactive](https://docs.google.com/document/d/1GGiI9-7KeY3TPqS3YT271upUVimo-XiL5mwWorDUD4c/edit):

* Representative
    * We should eventually do an ablation study, similar to the page load ablation study [here](https://docs.google.com/document/d/1wpu8aqZIUVgjNm9zBP9gU_swx5ODleH1s2Kueo1pIfc/edit#).

* Accurate
    * Summary [here](https://docs.google.com/document/d/1GGiI9-7KeY3TPqS3YT271upUVimo-XiL5mwWorDUD4c/edit#heading=h.iqlwzaf6lqrh), analysis [here](https://docs.google.com/document/d/1pZsTKqcBUb1pc49J89QbZDisCmHLpMyUqElOwYqTpSI/edit#bookmark=id.4euqu19nka18). Overall, based on manual investigation of 25 sites, our approach fired uncontroversially at the right time 64% of the time, and possibly too late the other 36% of time. We split TTI in two to allow this metric to be quite pessimistic about when TTI fires, so we’re happy with when this fires for all 25 sites. A few issues with this research:
        * Ideally someone less familiar with our approach would have performed the evaluation.
        * Ideally we’d have looked at more than 25 sites.
* Stable
    * Analysis [here](https://docs.google.com/document/d/1GGiI9-7KeY3TPqS3YT271upUVimo-XiL5mwWorDUD4c/edit#heading=h.27s41u6tkfzj).
* Interpretable
    * Time to Consistently Interactive is easy to explain. We report the first 5 second window where the network is roughly idle and no tasks are greater than 50ms long.
* Elastic
    * Time to Consistently Interactive is generally non-elastic. We’re investigating another metric which will quantify how busy the main thread is between FMP and TTI, which should be a nice elastic proxy metric for TTI.
* Simple
    * Time To Consistently Interactive has a reasonable amount of complexity, but is much simpler than Time to First Interactive. Time to Consistently Interactive has 3 parameters:
        * Number of allowable requests during network idle (currently 2).
        * Length of allowable tasks during main thread idle (currently 50ms).
        * Window length (currently 5 seconds).
* Realtime
    * Time To Consistently Interactive is definitely not realtime, as it needs to wait until it’s seen 5 seconds of idle time before declaring that we became interactive at the start of the 5 second window.
