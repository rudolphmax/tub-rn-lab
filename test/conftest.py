import pytest


def pytest_addoption(parser):
    parser.addoption('--executable', action='store', default='build/webserver')
    parser.addoption('--port', action='store', default=4711)
    parser.addoption('--timeout', action='store', default=2)


@pytest.fixture
def port(request):
    return request.config.getoption('port')


@pytest.fixture
def timeout(request):
    return request.config.getoption('timeout')
