import contextlib
import socket
import sys
import time
import urllib.request as req

import pytest

import dht
import util


STABILIZE_INTERVAL = 1.0


@pytest.fixture
def dynamic_peer(request):
    """Return a function for spawning DHT peers
    """
    def runner(peer, neighbors=None, anchor=None):
        """Spawn a dynamic peer

        A dynamic peer is expected to process join, stabilize, and notify messages correctly.
        It can be started with predefined `neighbors`, similar to a static peer.
        Alternatively, it receives an `anchor`-peer it tries to join, or becomes its own DHT.
        """
        assert not (neighbors and anchor), "Cannot start joining peer with neighborhood"

        if neighbors:
            predecessor, successor = neighbors
            args = []
            env = {
                'PRED_ID': f'{predecessor.id}', 'PRED_IP': predecessor.ip, 'PRED_PORT': f'{predecessor.port}',
                'SUCC_ID': f'{successor.id}', 'SUCC_IP': successor.ip, 'SUCC_PORT': f'{successor.port}',
            }
        elif anchor:
            args = [anchor.ip, f'{anchor.port}']
            env = {}
        else:
            args = []
            env = {}

        return util.KillOnExit(
            [request.config.getoption('executable'), peer.ip, f'{peer.port}', f'{peer.id}'] + args,
            env=env
        )

    return runner


def test_stabilize(dynamic_peer, timeout):
    """Ensure peers send stabilize messages"""

    predecessor = dht.Peer(0x0000, "127.0.0.1", 4710)
    self = dht.Peer(0x1000, "127.0.0.1", 4711)
    successor = dht.Peer(0x2000, "127.0.0.1", 4712)

    with dht.peer_socket(predecessor, timeout) as pred_mock, dynamic_peer(
        self, neighbors=(predecessor, successor)
    ), dht.peer_socket(successor, timeout) as succ_mock:

        time.sleep(STABILIZE_INTERVAL + 0.1)

        assert (
            util.bytes_available(pred_mock) == 0
        ), "Data received on predecessor socket"
        dht.expect_msg(succ_mock, dht.Message(dht.Flags.stabilize, None, self))


def test_notify(dynamic_peer, timeout):
    """Test whether peers respond correctly to stabilize"""

    previous_predecessor = dht.Peer(0xF000, "127.0.0.1", 4710)
    predecessor = dht.Peer(0x0000, "127.0.0.1", 4711)
    self = dht.Peer(0x1000, "127.0.0.1", 4712)
    successor = dht.Peer(0x2000, "127.0.0.1", 4713)

    with dht.peer_socket(
        previous_predecessor, timeout
    ) as prev_mock, dht.peer_socket(
        predecessor, timeout
    ) as pred_mock, dynamic_peer(
        self, neighbors=(predecessor, successor)
    ), dht.peer_socket(
        successor, timeout
    ):
        # Send stabilize
        stabilize = dht.Message(dht.Flags.stabilize, 0, previous_predecessor)
        prev_mock.sendto(dht.serialize(stabilize), (self.ip, self.port))

        time.sleep(0.1)

        # Expect notify
        dht.expect_msg(prev_mock, dht.Message(dht.Flags.notify, None, predecessor))

        assert (
            util.bytes_available(pred_mock) == 0
        ), "Data received on predecessor socket"


def test_successor_update(dynamic_peer, timeout):
    """Ensure that the peer modifies its successor when notified

    `successor` has just joined the DHT. `previous_successor` updated
    its predecessor accordingly.
    """

    predecessor = dht.Peer(0x0000, "127.0.0.1", 4710)
    self = dht.Peer(0x1000, "127.0.0.1", 4711)
    successor = dht.Peer(0x2000, "127.0.0.1", 4712)
    previous_successor = dht.Peer(0x3000, "127.0.0.1", 4713)

    with dht.peer_socket(
        predecessor, timeout
    ) as pred_mock, dynamic_peer(
        self, neighbors=(predecessor, previous_successor)
    ), dht.peer_socket(
        successor, timeout
    ) as succ_mock, dht.peer_socket(
        previous_successor, timeout
    ) as prev_mock:
        prev_mock.recv(1024)  # Await first stabilize

        # Send notify
        notify = dht.Message(dht.Flags.notify, 0, successor)
        prev_mock.sendto(dht.serialize(notify), (self.ip, self.port))

        time.sleep(STABILIZE_INTERVAL + 0.1)  # Wait for next stabilize
        dht.expect_msg(succ_mock, dht.Message(dht.Flags.stabilize, None, self))

        assert (
            util.bytes_available(pred_mock) == 0
        ), "Data received on predecessor socket"
        assert (
            util.bytes_available(pred_mock) == 0
        ), "Data received on previous succeessor socket"


def test_join(dynamic_peer, timeout):
    """Confirm that nodes started with reference to peer join the DHT"""

    self = dht.Peer(0x1000, "127.0.0.1", 4711)
    anchor = dht.Peer(0x2000, "127.0.0.1", 4712)

    with dht.peer_socket(anchor, timeout) as anchor_mock:
        with dynamic_peer(
                self, anchor=anchor
        ):
            time.sleep(0.1)

        dht.expect_msg(anchor_mock, dht.Message(dht.Flags.join, None, self))


def test_join_forward(dynamic_peer, timeout):
    """Ensure that join packets are forwarded"""

    predecessor = dht.Peer(0x0000, "127.0.0.1", 4710)
    self = dht.Peer(0x1000, "127.0.0.1", 4711)
    successor = dht.Peer(0x2000, "127.0.0.1", 4712)
    joining = dht.Peer(0x3000, "127.0.0.1", 4713)

    with dht.peer_socket(
        predecessor, timeout
    ) as pred_mock, dynamic_peer(
        self, neighbors=(predecessor, successor)
    ), dht.peer_socket(
        successor, timeout
    ) as succ_mock, dht.peer_socket(
        joining, timeout
    ) as join_mock:
        # Send join
        join = dht.Message(dht.Flags.join, 0, joining)
        pred_mock.sendto(dht.serialize(join), (self.ip, self.port))

        time.sleep(0.1)

        # Receive forwarded join
        dht.expect_msg(succ_mock, dht.Message(dht.Flags.join, None, joining))

        assert (
            util.bytes_available(pred_mock) == 0
        ), "Data received on predecessor socket"
        assert util.bytes_available(join_mock) == 0, "Data received on joining socket"


def test_join_react(dynamic_peer, timeout):
    """Check that the joining peer is notified"""

    predecessor = dht.Peer(0x0000, "127.0.0.1", 4710)
    self = dht.Peer(0x2000, "127.0.0.1", 4711)
    successor = dht.Peer(0x3000, "127.0.0.1", 4712)
    joining = dht.Peer(0x1000, "127.0.0.1", 4713)

    with dht.peer_socket(
        predecessor, timeout
    ) as pred_mock, dynamic_peer(
        self, neighbors=(predecessor, successor)
    ), dht.peer_socket(
        successor, timeout
    ), dht.peer_socket(
        joining, timeout
    ) as join_mock:
        # Send join
        join = dht.Message(dht.Flags.join, 0, joining)
        pred_mock.sendto(dht.serialize(join), (self.ip, self.port))

        time.sleep(0.1)

        # Receive notify
        dht.expect_msg(join_mock, dht.Message(dht.Flags.notify, None, self))


def test_dht(dynamic_peer):
    """Test a complete DHT

    Now we can test a DHT that is dynamically created by joining nodes consecutively.
    Again, we will repeat the dynamic content test, contacting a different peer for
    each request.

    The test starts five peers with random IDs, with each new one joining the DHT via
    the last peer that was started.
    """

    # chosen by fair dice roll.
    # guaranteed to be random.
    dht_ids = [7044, 15792, 59957, 4386, 18648]
    datum = "b23464529c2c4697"
    content = b"cdd3b63981a25b09820b40aaef2e9405b9fef79e9d2b922ebc2cd5d68b0527de"

    peers = [dht.Peer(id_, "127.0.0.1", 4710 + i) for i, id_ in enumerate(dht_ids)]

    with contextlib.ExitStack() as contexts:

        # Start the first node
        peer = peers[0]
        contexts.enter_context(dynamic_peer(peer))
        time.sleep(0.1)

        for anchor, joining in zip(peers[:-1], peers[1:]):
            contexts.enter_context(dynamic_peer(joining, anchor=anchor))
            time.sleep(3 * STABILIZE_INTERVAL)

        # Ensure datum is missing
        contact = peers[0]
        with pytest.raises(req.HTTPError) as exception_info:
            util.urlopen(f"http://{contact.ip}:{contact.port}/dynamic/{datum}")

        assert (
            exception_info.value.status == 404
        ), f"'/dynamic/{datum}' should be missing, but GET was not answered with '404'"

        # Create datum
        contact = peers[1]
        with pytest.raises(req.HTTPError) as exception_info:
            util.urlopen(f"http://{contact.ip}:{contact.port}/dynamic/{datum}")
        real_url = exception_info.value.geturl()  # Determine correct URL
        reply = util.urlopen(req.Request(real_url, data=content, method="PUT"))
        assert (
            reply.status == 201
        ), f"Creation of '/dynamic/{datum}' did not yield '201'"

        # Ensure datum exists
        contact = peers[2]
        reply = util.urlopen(f"http://{contact.ip}:{contact.port}/dynamic/{datum}")
        assert reply.status == 200
        assert (
            reply.read() == content
        ), f"Content of '/dynamic/{datum}' does not match what was passed"

        # Delete datum
        contact = peers[3]
        real_url = util.urlopen(
            f"http://{contact.ip}:{contact.port}/dynamic/{datum}"
        ).geturl()  # Determine correct URL
        reply = util.urlopen(req.Request(real_url, method="DELETE"))
        assert reply.status in {200, 202, 204}, f"Deletion of '/dynamic/{datum}' did not succeed"

        # Ensure datum is missing
        contact = peers[4]
        with pytest.raises(req.HTTPError) as exception_info:
            util.urlopen(f"http://{contact.ip}:{contact.port}/dynamic/{datum}")

        assert (
            exception_info.value.status == 404
        ), f"'/dynamic/{datum}' should be missing, but GET was not answered with '404'"
