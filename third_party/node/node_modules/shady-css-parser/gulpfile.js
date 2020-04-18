'use strict';
var gulp = require('gulp');
var babel = require('gulp-babel');
var concat = require('gulp-concat');
var uglify = require('gulp-uglify');
var size = require('gulp-size');
var rename = require('gulp-rename');
var mocha = require('gulp-mocha');
var del = require('del');

require('babel-core/register');

var src = ['src/**/*.js'];
var dist = 'dist';
var concatOrder = [
  '/shady-css/common.js',
  '/shady-css/node-factory.js',
  '/shady-css/node-visitor.js',
  '/shady-css/stringifier.js',
  '/shady-css/token.js',
  '/shady-css/tokenizer.js',
  '/shady-css/parser.js',
  '/*.js'
].map(function(path) {
  return 'src' + path;
});
var measureable = [
  dist + '/shady-css.concat.js',
  dist + '/shady-css.min.js'
];

gulp.task('default', ['clean', 'test', 'build', 'minify', 'measure']);

gulp.task('build', ['test', 'transform', 'minify']);

gulp.task('concat', ['test'], function() {
  return gulp.src(concatOrder)
    .pipe(babel())
    .on('error', function(error) {
      console.error(error);
    })
    .pipe(concat('shady-css.concat.js'))
    .pipe(gulp.dest(dist));
});

gulp.task('transform', ['test'], function() {
  return gulp.src(src)
    .pipe(babel())
    .on('error', function(error) {
      console.error(error);
    })
    .pipe(gulp.dest(dist));
});

gulp.task('minify', ['concat'], function() {
  return gulp.src(dist + '/shady-css.concat.js')
    .pipe(uglify())
    .on('error', function(error) {
      console.error(error);
    })
    .pipe(rename('shady-css.min.js'))
    .pipe(gulp.dest(dist));
});

gulp.task('measure', ['minify'], function() {
  return gulp.src(measureable)
    .pipe(size({
      showFiles: true
    }))
    .pipe(size({
      showFiles: true,
      gzip: true
    }));
});

gulp.task('test', function() {
  return gulp.src('test/*.js', {
    read: false
  }).pipe(mocha());
});

gulp.task('clean', function() {
  del.sync([dist]);
});

gulp.task('watch', function(done) {
  gulp.watch(src, ['default', 'minify', 'measure']);
});
