"""UC8176 (IL0398) e-paper controller.

Panels:
- GDEW042M01 (Good Display): 400x300 monochrome
- GDEW042Z15 (Good Display): 400x300 black/white/red (full refresh only)

The mono panel's partial-refresh register LUTs (0x20-0x24) are panel data,
supplied here. UC8176 BUSY is active-LOW; the driver handles that.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.mipi import flatten_sequence
from esphome.core import ID

from ..display import CONF_INIT_SEQUENCE_ID
from . import RequiredPinsModel

# GDEW042M01 partial LUT phase timings (from GxEPD2 GxEPD2_420_M01).
_T1, _T2, _T3, _T4, _T5, _T6 = 20, 20, 40, 40, 3, 3
_LUT_VCOM = [0x00, _T1, _T2, _T3, _T4, 0x01] + [0x00] * 38  # 0x20
_LUT_WW = [0x02, _T1, _T2, _T3, _T5, 0x01] + [0x00] * 36  # 0x21
_LUT_BW = [0x5A, _T1, _T2, _T3, _T4, 0x01] + [0x00] * 36  # 0x22
_LUT_WB = [0x84, _T1, _T2, _T3, _T4, 0x01] + [0x00] * 36  # 0x23
_LUT_BB = [0x01, _T1, _T2, _T3, _T6, 0x01] + [0x00] * 36  # 0x24


class UC8176Mono(RequiredPinsModel):
    def __init__(
        self,
        name,
        luts=(_LUT_VCOM, _LUT_WW, _LUT_BW, _LUT_WB, _LUT_BB),
        pll=0x3A,
        vcom_dc=0x1A,
        **defaults,
    ):
        super().__init__(name, "EPaperUC8176", **defaults)
        self.luts = luts
        self.pll = pll
        self.vcom_dc = vcom_dc

    def get_init_sequence(self, config):
        width, height = self.get_dimensions(config)
        return (
            (0x00, 0x1F, 0x0D),  # panel setting (KW, LUT from OTP)
            (
                0x61,
                (width >> 8) & 0xFF,
                width & 0xFF,
                (height >> 8) & 0xFF,
                height & 0xFF,
            ),  # TRES (resolution)
            (0x50, 0x97),  # VCOM and data interval setting
        )

    def get_constructor_args(self, config) -> tuple:
        width, height = self.get_dimensions(config)
        lut_vcom, lut_ww, lut_bw, lut_wb, lut_bb = self.luts
        partial = (
            (0x01, 0x03, 0x00, 0x2B, 0x2B),  # power setting
            (0x06, 0x17, 0x17, 0x17),  # booster soft start
            (0x00, 0x3F),  # panel setting (LUT from register)
            (0x30, self.pll),  # PLL control
            (
                0x61,
                (width >> 8) & 0xFF,
                width & 0xFF,
                (height >> 8) & 0xFF,
                height & 0xFF,
            ),  # TRES (resolution)
            (0x82, self.vcom_dc),  # VCOM DC setting
            (0x50, 0xD7),  # VCOM/interval, border floating
            (0x20, *lut_vcom),
            (0x21, *lut_ww),
            (0x22, *lut_bw),
            (0x23, *lut_wb),
            (0x24, *lut_bb),
        )
        flat = flatten_sequence(partial)
        arr = cg.static_const_array(
            ID(config[CONF_INIT_SEQUENCE_ID].id + "_partial", type=cg.uint8), flat
        )
        return (arr, len(flat))


class UC8176BWR(RequiredPinsModel):
    """3-color, full refresh only (resolution comes from OTP)."""

    def __init__(self, name, **defaults):
        super().__init__(name, "EPaperUC8176BWR", **defaults)

    def get_init_sequence(self, config):
        return (
            (0x06, 0x17, 0x17, 0x17),  # booster soft start
            (0x00, 0x0F, 0x0D),  # panel setting (KWR, LUT from OTP)
        )


UC8176Mono(
    "GDEW042M01", width=400, height=300, data_rate="2MHz", minimum_update_interval="1s"
)
UC8176BWR(
    "GDEW042Z15", width=400, height=300, data_rate="2MHz", minimum_update_interval="30s"
)


# --- Lokale Ergänzung (homeassistant-epaper) -------------------------------------------
# Waveshare 4.2" e-Paper V1 (Good Display GDEW042T2, UC8176). Partial-Waveform aus GxEPD2
# (GxEPD2_420.cpp, J.-M. Zingg, für genau dieses Panel entwickelt), dazu PLL 0x3A und
# VCOM-DC 0x12 (-1,0 V) wie dort.
#   T1 Ladungsausgleich vor dem Farbwechsel, T2/T3 Erhaltungsphase (hält unveränderte
#   Pixel satt), T4 Farbwechsel. Einheit: Frames der PLL-Frequenz (100 Hz).


def _gdew042t2_luts(t1, t2, t3, t4):
    def lut(level, size):
        return [level, t1, t2, t3, t4, 0x01] + [0x00] * (size - 6)

    return (
        lut(0x00, 44),  # 0x20 VCOM         00 00 00 00
        lut(0x18, 42),  # 0x21 WW (weiß bleibt)    00 01 10 00
        lut(0x5A, 42),  # 0x22 BW (wird weiß)      01 01 10 10
        lut(0xA5, 42),  # 0x23 WB (wird schwarz)   10 10 01 01
        lut(0x24, 42),  # 0x24 BB (schwarz bleibt) 00 10 01 00
    )


_WS42V1 = dict(
    width=400, height=300, data_rate="2MHz", minimum_update_interval="1s", vcom_dc=0x12
)

# GxEPD2-Standard: Ausgleich, kurze Erhaltung, Farbwechsel
UC8176Mono("waveshare-4.2in-v1", luts=_gdew042t2_luts(25, 1, 4, 25), **_WS42V1)
# GxEPD2-Variante "balanced flash once": Teilbereich blinkt einmal kurz
UC8176Mono("waveshare-4.2in-v1-flash", luts=_gdew042t2_luts(0, 25, 25, 0), **_WS42V1)


# Einstellbare Variante mit den Pegeln der GDEW042M01-Waveform (im Test besser als die GxEPD2-Werte).
#   partial_timing: [T1, T2, T3, T4, T5, T6]  Frames je Phase; T1-T4 für alle Tabellen,
#                   T5 = Nachtreiben "weiß bleibt" (WW), T6 = Nachtreiben "schwarz bleibt" (BB)
#   vcom_dc:        0x82-Register, VCOM = -0,1 V - 0,05 V * Wert (0x1A = -1,4 V, 0x12 = -1,0 V)
#   pll:            0x30-Register, 0x3A = 100 Hz, 0x3C = 50 Hz (langsamer, kräftiger)
CONF_PARTIAL_TIMING = "partial_timing"
CONF_VCOM_DC = "vcom_dc"
CONF_PLL = "pll"


def _m01_luts(t1, t2, t3, t4, t5, t6):
    return (
        [0x00, t1, t2, t3, t4, 0x01] + [0x00] * 38,  # 0x20 VCOM
        [0x02, t1, t2, t3, t5, 0x01] + [0x00] * 36,  # 0x21 WW
        [0x5A, t1, t2, t3, t4, 0x01] + [0x00] * 36,  # 0x22 BW
        [0x84, t1, t2, t3, t4, 0x01] + [0x00] * 36,  # 0x23 WB
        [0x01, t1, t2, t3, t6, 0x01] + [0x00] * 36,  # 0x24 BB
    )


class UC8176MonoTunable(UC8176Mono):
    def extra_schema(self) -> dict:
        return {
            cv.Optional(CONF_PARTIAL_TIMING, default=[20, 20, 40, 40, 3, 3]): cv.All(
                cv.ensure_list(cv.int_range(0, 255)), cv.Length(min=6, max=6)
            ),
            cv.Optional(CONF_VCOM_DC, default=0x1A): cv.int_range(0, 0x3A),
            cv.Optional(CONF_PLL, default=0x3A): cv.int_range(0, 0x3F),
        }

    def get_constructor_args(self, config) -> tuple:
        self.luts = _m01_luts(*config[CONF_PARTIAL_TIMING])
        self.vcom_dc = config[CONF_VCOM_DC]
        self.pll = config[CONF_PLL]
        return super().get_constructor_args(config)


UC8176MonoTunable(
    "waveshare-4.2in-v1-tune",
    width=400,
    height=300,
    data_rate="2MHz",
    minimum_update_interval="1s",
)
