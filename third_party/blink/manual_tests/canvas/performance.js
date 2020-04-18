// Canvas performance library
/*
TODO:
  - add interface for performance
  - support scrolling
  -

*/

import "./utils.js";

const bar = document.createElement("div");
const barTime = document.createElement("div");

function draw() {
  if (perf.timers.length == 0) {
    const t = perf.coldTime - perf.lastTime;
    barTime.innerHTML = `warming up [${perf.ntos(t)}]`;
  } else {
    let [average, conf] = perf.stats();
    let t = perf.lastTime - perf.firstTime;

    barTime.innerHTML = `avg: ${perf.ntoserr(average, conf)} [${perf.ntos(t)}]`;
  }

  requestAnimationFrame(draw);
}

function setupDraw() {
  bar.style.position = "fixed";
  bar.style.backgroundColor = "#444";
  bar.style.color = "#EEE";
  bar.style.bottom = "0";
  bar.style.width = "600px";
  bar.style.left = "50%";
  bar.style.marginLeft = "-302px";
  bar.style.fontFamily = "arial";
  bar.style.padding = "10px 4px";
  bar.style.textAlign = "center";

  barTime.style.width = "600px";
  barTime.style.display = "inline-block";

  bar.appendChild(barTime);
  document.body.appendChild(bar);
  draw();
};

let scrollDown = true;
let scrollRight = true;
let xpos = 0, ypos = 0;
function scrollPage() {
  let height = document.body.scrollHeight - window.innerHeight;
  let width = document.body.scrollWidth - window.innerWidth;

  if (scrollDown) {
    ypos += 10;
    if (ypos >= height) scrollDown = false;
  } else {
    ypos -= 10;
    if (ypos <= 0) scrollDown = true;
  }
  if (scrollRight) {
    xpos += 10;
    if (xpos >= width) scrollRight = false;
  } else {
    xpos -= 10;
    if (xpos <= 0) scrollRight = true;
  }
  window.scrollTo(xpos, ypos);

  requestAnimationFrame(scrollPage);
}

let scroll = false;

document.addEventListener("DOMContentLoaded", (ev) => {
  if (!perf.onContinuousMode()) {
    setupDraw();
  }
  perf.measure();
  scroll = (window.location.search == "?scroll");

  if (scroll) {
    scrollPage();
  }
});
