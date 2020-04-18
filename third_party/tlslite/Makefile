# Authors: 
#   Trevor Perrin
#   Hubert Kario - test and test-dev
#
.PHONY : default
default:
	@echo To install tlslite run \"./setup.py install\" or \"make install\"

.PHONY: install
install:
	./setup.py install

.PHONY : clean
clean:
	rm -rf tlslite/*.pyc
	rm -rf tlslite/utils/*.pyc
	rm -rf tlslite/integration/*.pyc	
	rm -rf dist
	rm -rf docs
	rm -rf build
	rm -f MANIFEST

docs:
	epydoc --html -v --introspect-only -o docs tlslite

dist: docs
	./setup.py sdist

test:
	cd tests/ && python ./tlstest.py server localhost:4433 . & sleep 1
	cd tests/ && python ./tlstest.py client localhost:4433 .

test-dev:
	cd tests/ && PYTHONPATH=.. python ./tlstest.py server localhost:4433 . & sleep 1
	cd tests/ && PYTHONPATH=.. python ./tlstest.py client localhost:4433 .
