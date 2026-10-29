import importlib.util
from pathlib import Path
import unittest
import struct
spec=importlib.util.spec_from_file_location('tx',Path(__file__).parents[1]/'tools/be_tx_descriptor.py')
tx=importlib.util.module_from_spec(spec)
spec.loader.exec_module(tx)


class TxDescriptorTest(unittest.TestCase):
    def test_golden_little_endian(self):
        body=tx.encode_body(dict(ch_dma=8,pkt_size=24,hdr_llc_len=12,qsel=18,seq=0x123,
                                 data_rate=4,use_rate=1))
        self.assertEqual(body.hex(), '0064080000000000180024002301000000000000000000000000000000000480')
        self.assertEqual(len(body),32)

    def test_reject_truncation_and_unsupported_paths(self):
        for key,(_,_,width) in tx.FIELDS.items():
            for bad in (-1,1<<width,True,1.5):
                with self.assertRaises(ValueError):
                    tx.encode_body({'ch_dma':8,'pkt_size':24, key:bad})
        for extra in ({'sec_type':1},{'en_wd_info':1},{'ch_dma':12},{'pkt_size':0}):
            with self.assertRaises(ValueError):tx.encode_body({**dict(ch_dma=8,pkt_size=24),**extra})
        with self.assertRaises(ValueError):tx.encode_body({'pkt_size':24})

    def test_be_macid_and_optional_words_remain_distinct(self):
        words=struct.unpack('<8I',tx.encode_body(dict(ch_dma=8,pkt_size=24,mac_id=255,seq=4095)))
        self.assertEqual(words[2],0xff000018)
        self.assertEqual(words[3],4095)
        self.assertEqual(words[1],0)
        self.assertEqual(words[4:7],(0,0,0))

    def test_wdinfo_golden_and_coupled_enable_bit(self):
        fields=dict(is_bmc=0,port=3,stbc=1,ldpc=1,dis_data_fb=1,tx_cnt_lmt=2,
                    tx_cnt_lmt_en=1,ampdu_num=3,a_ctrl_bsr=1,
                    data_retry_lowest_rate=4,sn=10,ampdu_density=2,report=1)
        self.assertEqual(tx.encode_info(fields).hex(),
                         '301c4200034004a0000008400000000000000088000000000000000000000000')
        descriptor=tx.encode_descriptor(dict(ch_dma=8,pkt_size=24),fields)
        self.assertEqual(len(descriptor),64)
        self.assertEqual(struct.unpack_from('<I',descriptor)[0],0x00480400)
        self.assertEqual(descriptor[32:],tx.encode_info(fields))
        bmc=struct.unpack('<8I',tx.encode_info(dict(is_bmc=1)))
        self.assertEqual(bmc[4],0x80000000)

    def test_wdinfo_field_boundaries_and_no_encryption(self):
        for name,(_,_,width) in tx.INFO_FIELDS.items():
            for bad in (-1,1<<width,True,1.5):
                with self.assertRaises(ValueError):tx.encode_info({'is_bmc':0,name:bad})
        for fields in ({},{'is_bmc':True},{'is_bmc':2},{'is_bmc':0,'sec_en':1}):
            with self.assertRaises(ValueError):tx.encode_info(fields)
