import {writeFile, mkdirSync} from 'fs';
import gulp from 'gulp';
import del from 'del';
import sequence from 'run-sequence';
import {rollup} from 'rollup';
import nodeResolve from 'rollup-plugin-node-resolve';
import commonjs from 'rollup-plugin-commonjs';
import babelRollup from 'rollup-plugin-babel';
import {Bundler} from 'polymer-bundler';
import {serialize} from 'parse5';
import pify from 'pify';

gulp.task('default', ['dist']);

gulp.task('clean', () => del(['lib', 'dist']));

gulp.task('build', done => sequence(['clean'], ['build:lib', 'build:copy'], done));

gulp.task('build:lib', () => {
	return rollup({
		entry: 'src/index.js',
		plugins: [
			nodeResolve({
				jsnext: true,
				main: true
			}),
			commonjs({
				include: 'node_modules/**'
			}),
			babelRollup({
				babelrc: false,
				presets: [['env', {
					targets: {
						browsers: [
							'last 2 versions',
							'not ie 10'
						]
					},
					modules: false,
					exclude: [
						'transform-es2015-classes'
					]
				}]],
				plugins: [
					'transform-object-assign'
				]
			})
		]
	}).then(bundle => {
		return bundle.write({
			format: 'iife',
			moduleName: 'PolymerRedux',
			dest: 'lib/index.js'
		});
	});
});

gulp.task('build:copy', () => gulp.src(['src/**.html']).pipe(gulp.dest('lib')));

gulp.task('dist', ['build'], () => {
	const entry = 'lib/index.html';
	const b = new Bundler({
		inlineScripts: true
	});

	return b.generateManifest([entry])
		.then(manifest => b.bundle(manifest))
		.then(bundles => {
			const {ast} = bundles.documents.get(entry);
			const html = serialize(ast);

			mkdirSync('dist');

			return pify(writeFile)('dist/polymer-redux.html', `${html}\n`);
		});
});

gulp.task('develop', ['dist'], () => {
	gulp.watch(['src/**/*'], ['dist']);
});
