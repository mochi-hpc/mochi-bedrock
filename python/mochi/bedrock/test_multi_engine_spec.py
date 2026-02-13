import unittest
import json
import mochi.bedrock.server as mbs
import mochi.bedrock.spec as spec


class TestProcSpecMultiEngine(unittest.TestCase):
    """Tests for ProcSpec with multi-engine (list of MargoSpec) support."""

    def test_single_margo_string(self):
        """Single margo string still works (backward compat)."""
        p = spec.ProcSpec(margo="na+sm")
        self.assertIsInstance(p.margo, spec.MargoSpec)

    def test_single_margo_spec(self):
        """Single MargoSpec still works (backward compat)."""
        m = spec.MargoSpec(mercury="na+sm")
        p = spec.ProcSpec(margo=m)
        self.assertIsInstance(p.margo, spec.MargoSpec)
        self.assertIs(p.margo, m)

    def test_list_of_strings(self):
        """List of address strings produces list of MargoSpecs."""
        p = spec.ProcSpec(margo=["na+sm", "na+sm"])
        self.assertIsInstance(p.margo, list)
        self.assertEqual(len(p.margo), 2)
        for m in p.margo:
            self.assertIsInstance(m, spec.MargoSpec)

    def test_list_of_margo_specs(self):
        """List of MargoSpec instances works."""
        m1 = spec.MargoSpec(mercury="na+sm")
        m2 = spec.MargoSpec(mercury="na+sm")
        p = spec.ProcSpec(margo=[m1, m2])
        self.assertIsInstance(p.margo, list)
        self.assertEqual(len(p.margo), 2)

    def test_empty_list_raises(self):
        """Empty list raises ValueError."""
        with self.assertRaises(ValueError):
            spec.ProcSpec(margo=[])

    def test_bedrock_default_with_list(self):
        """Bedrock spec defaults to first margo's rpc_pool when margo is a list."""
        p = spec.ProcSpec(margo=["na+sm", "na+sm"])
        self.assertIsInstance(p.bedrock, spec.BedrockSpec)
        self.assertEqual(p.bedrock.pool, p.margo[0].rpc_pool)

    def test_to_dict_single(self):
        """to_dict with single margo produces dict for margo field."""
        p = spec.ProcSpec(margo="na+sm")
        d = p.to_dict()
        self.assertIsInstance(d['margo'], dict)

    def test_to_dict_list(self):
        """to_dict with list margo produces list for margo field."""
        p = spec.ProcSpec(margo=["na+sm", "na+sm"])
        d = p.to_dict()
        self.assertIsInstance(d['margo'], list)
        self.assertEqual(len(d['margo']), 2)
        for m in d['margo']:
            self.assertIsInstance(m, dict)

    def test_from_dict_single(self):
        """from_dict with single margo dict produces single MargoSpec."""
        p = spec.ProcSpec(margo="na+sm")
        d = p.to_dict()
        p2 = spec.ProcSpec.from_dict(d)
        self.assertIsInstance(p2.margo, spec.MargoSpec)

    def test_from_dict_list(self):
        """from_dict with list margo produces list of MargoSpecs."""
        p = spec.ProcSpec(margo=["na+sm", "na+sm"])
        d = p.to_dict()
        p2 = spec.ProcSpec.from_dict(d)
        self.assertIsInstance(p2.margo, list)
        self.assertEqual(len(p2.margo), 2)

    def test_roundtrip_single(self):
        """JSON roundtrip with single margo."""
        p = spec.ProcSpec(margo="na+sm")
        j = p.to_json()
        p2 = spec.ProcSpec.from_json(j)
        self.assertIsInstance(p2.margo, spec.MargoSpec)
        self.assertEqual(p.to_dict(), p2.to_dict())

    def test_roundtrip_list(self):
        """JSON roundtrip with list margo."""
        p = spec.ProcSpec(margo=["na+sm", "na+sm"])
        j = p.to_json()
        p2 = spec.ProcSpec.from_json(j)
        self.assertIsInstance(p2.margo, list)
        self.assertEqual(len(p2.margo), 2)
        self.assertEqual(p.to_dict(), p2.to_dict())


class TestServerMultiEngineSpec(unittest.TestCase):
    """Integration tests for multi-engine with the actual Server."""

    def test_server_multi_engine_from_config(self):
        """Server with margo array config creates multiple engines."""
        config = {"margo": [{}, {}]}
        server = mbs.Server(address="na+sm", config=config)
        self.assertEqual(server.margo.num_engines, 2)
        # Verify the config output has margo as an array
        output = server.config
        self.assertIsInstance(output['margo'], list)
        self.assertEqual(len(output['margo']), 2)
        # Each margo element should be a dict with argobots config
        for m in output['margo']:
            self.assertIsInstance(m, dict)
            self.assertIn('argobots', m)
        server.finalize()

    def test_server_single_engine_config(self):
        """Server with single-engine config output has margo as dict."""
        server = mbs.Server(address="na+sm")
        self.assertEqual(server.margo.num_engines, 1)
        output = server.config
        self.assertIsInstance(output['margo'], dict)
        server.finalize()


if __name__ == '__main__':
    unittest.main()
