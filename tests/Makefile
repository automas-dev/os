
DARK_MODE=OFF

test: build
	cd build && GTEST_COLOR=1 ctest --output-on-failure

test_cov: test coverage

coverage:
	@mkdir -p build/coverage
	gcovr --txt \
		--html build/coverage/index.html \
		--html-details \
		--html-theme github.dark-green \
		--html-high-threshold 100 \
		--html-medium-threshold 90 \
		--fail-under-line 100 \
		--fail-under-branch 100 \
		--lcov build/coverage.lcov \
		-r ../ \
		-e $(CURDIR) \
		-e ../build \
		--object-directory=$(PWD)/build

test-ci: build
	cd build && GTEST_COLOR=1 ctest -V

setup:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DOS_TEST_COVERAGE_DARK=${DARK_MODE}

build: setup
	cmake --build build -j

clean:
	cmake --build build --target clean

.PHONY: setup build clean test test_cov coverage test-ci
