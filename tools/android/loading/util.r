# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Useful R routines for analyzing output from several cost_to_csv.py
# output and producing interesting graphs.

combine.runs <- function(times, prefix, suffix)
  do.call("rbind", lapply(times, function (t)
    with(read.csv(paste0(prefix, t, suffix)),
         data.frame(site, kind, cost, time=t))))

get.ordered.names <- function(runs) {
  means <- with(runs, tapply(cost, list(site, kind), mean))
  return(names(means[,"cold"])[order(means[,"cold"])])
}

plot.warm.cold <- function(runs, main="") {
  ordered.names <- get.ordered.names(runs)
  n <- length(ordered.names)
  par(mar=c(8,4,4,4), bg="white")
  plot(NULL, xlim=c(1,25), ylim=range(runs$cost), xaxt="n",
       ylab="ms", xlab="", main=main)
  axis(1, 1:n, labels=ordered.names, las=2)
  getdata <- function(k, t) sapply(
      ordered.names, function (s) with(runs, cost[site==s & kind==k & time==t]))
  for (t in unique(runs$time)) {
    points(1:n, getdata("cold", t), pch=1)
    points(1:n, getdata("warm", t), pch=3)
  }
  legend("topleft", pch=c(1, 3), legend=c("cold", "warm"))
}

plot.relative.sds <- function(runs, main="") {
  sds <- with(runs, tapply(cost, list(site, kind), sd))
  means <- with(runs, tapply(cost, list(site, kind), mean))
  ordered.names <- get.ordered.names(runs)
  n <- length(ordered.names)
  par(mar=c(8,4,4,4), bg="white")
  plot(NULL, xlim=c(1,25), ylim=c(0,.8),
       xaxt="n", ylab="Relative SD", xlab="", main=main)
  axis(1, 1:n, labels=ordered.names, las=2)
  getdata <- function(k) sapply(ordered.names, function(s) (sds/means)[s, k])
  points(1:n, getdata("cold"), pch=1)
  points(1:n, getdata("warm"), pch=3)
  legend("topleft", pch=c(1, 3), legend=c("cold", "warm"))
}
