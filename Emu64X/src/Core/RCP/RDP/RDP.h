#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class RCP;

	// =========================================================================
	//  RDP Command Register Layouts
	//  Based on: https://n64brew.dev/wiki/Reality_Display_Processor/Commands
	//  Fields are defined as (name, start_bit, end_bit) within the 64-bit word.
	//  Bit 0 is the LSB of the 64-bit command word.
	// =========================================================================

	// --- 0x08-0x0F: Fill Triangle (base command, 4 words = 2 dwords) ---
	// Word 0 (bits 63..32 of first dword)
	// Opcode bits 61..56:
	//   61..59 = 001 (triangle group)
	//   58     = shade
	//   57     = texture
	//   56     = zbuffer
	#define RDP_TRI_WORD0_FIELDS(M) \
		M(cmd_base,  59, 61) \
		M(shade,     58, 58) \
		M(texture,   57, 57) \
		M(zbuffer,   56, 56) \
		M(lft,       55, 55) \
		M(level,     51, 53) \
		M(tile,      48, 50) \
		M(YL,        32, 45) \
		M(YM,        16, 29) \
		M(YH,         0, 13)

	// Word 1 (bits 31..0 of first dword + bits 63..32 of second dword)
	#define RDP_TRI_WORD1_FIELDS(M) \
		M(XL,        32, 59) \
		M(DxLDy,      0, 29)

	// Word 2
	#define RDP_TRI_WORD2_FIELDS(M) \
		M(XH,        32, 59) \
		M(DxHDy,      0, 29)

	// Word 3
	#define RDP_TRI_WORD3_FIELDS(M) \
		M(XM,        32, 59) \
		M(DxMDy,      0, 29)

	DEFINE_REGISTER_LAYOUT(RDP_Tri_Word0_Register, U64, RDP_TRI_WORD0_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Word1_Register, U64, RDP_TRI_WORD1_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Word2_Register, U64, RDP_TRI_WORD2_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Word3_Register, U64, RDP_TRI_WORD3_FIELDS)

	// --- 0x24/0x25: Texture Rectangle (2 dwords) ---
	#define RDP_TEXRECT_WORD0_FIELDS(M) \
		M(cmd,       56, 61) \
		M(XL,        44, 55) \
		M(YL,        32, 43) \
		M(tile,      24, 26) \
		M(XH,        12, 23) \
		M(YH,         0, 11)

	#define RDP_TEXRECT_WORD1_FIELDS(M) \
		M(S,         48, 63) \
		M(T,         32, 47) \
		M(DsDx,      16, 31) \
		M(DtDy,       0, 15)

	DEFINE_REGISTER_LAYOUT(RDP_TexRect_Word0_Register, U64, RDP_TEXRECT_WORD0_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_TexRect_Word1_Register, U64, RDP_TEXRECT_WORD1_FIELDS)

	// --- 0x2A: Set Key GB ---
	#define RDP_SET_KEY_GB_FIELDS(M) \
		M(cmd,       56, 61) \
		M(width_g,   44, 55) \
		M(width_b,   32, 43) \
		M(center_g,  24, 31) \
		M(scale_g,   16, 23) \
		M(center_b,   8, 15) \
		M(scale_b,    0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetKeyGB_Register, U64, RDP_SET_KEY_GB_FIELDS)

	// --- 0x2B: Set Key R ---
	#define RDP_SET_KEY_R_FIELDS(M) \
		M(cmd,       56, 61) \
		M(width_r,   44, 55) \
		M(center_r,   8, 15) \
		M(scale_r,    0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetKeyR_Register, U64, RDP_SET_KEY_R_FIELDS)

	// --- 0x2C: Set Convert ---
	#define RDP_SET_CONVERT_FIELDS(M) \
		M(cmd,       56, 61) \
		M(k0,        45, 53) \
		M(k1,        36, 44) \
		M(k2,        27, 35) \
		M(k3,        18, 26) \
		M(k4,         9, 17) \
		M(k5,         0,  8)

	DEFINE_REGISTER_LAYOUT(RDP_SetConvert_Register, U64, RDP_SET_CONVERT_FIELDS)

	// --- 0x2D: Set Scissor ---
	#define RDP_SET_SCISSOR_FIELDS(M) \
		M(cmd,       56, 61) \
		M(XH,        44, 55) \
		M(YH,        32, 43) \
		M(f,         25, 25) \
		M(o,         24, 24) \
		M(XL,        12, 23) \
		M(YL,         0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_SetScissor_Register, U64, RDP_SET_SCISSOR_FIELDS)

	// --- 0x2E: Set Primitive Depth ---
	#define RDP_SET_PRIM_DEPTH_FIELDS(M) \
		M(cmd,       56, 61) \
		M(z,         16, 31) \
		M(dz,         0, 15)

	DEFINE_REGISTER_LAYOUT(RDP_SetPrimDepth_Register, U64, RDP_SET_PRIM_DEPTH_FIELDS)

	// --- 0x2F: Set Other Modes ---
	#define RDP_SET_OTHER_MODES_FIELDS(M) \
		M(cmd,            56, 61) \
		M(atomic_prim,    55, 55) \
		M(cycle_type,     52, 53) \
		M(persp_tex_en,   51, 51) \
		M(detail_tex_en,  50, 50) \
		M(sharpen_tex_en, 49, 49) \
		M(tex_lod_en,     48, 48) \
		M(en_tlut,        47, 47) \
		M(tlut_type,      46, 46) \
		M(sample_type,    45, 45) \
		M(mid_texel,      44, 44) \
		M(bi_lerp_0,      43, 43) \
		M(bi_lerp_1,      42, 42) \
		M(convert_one,    41, 41) \
		M(key_en,         40, 40) \
		M(rgb_dither_sel, 38, 39) \
		M(alpha_dither_sel,36, 37) \
		M(b_m1a_0,        30, 31) \
		M(b_m1a_1,        28, 29) \
		M(b_m1b_0,        26, 27) \
		M(b_m1b_1,        24, 25) \
		M(b_m2a_0,        22, 23) \
		M(b_m2a_1,        20, 21) \
		M(b_m2b_0,        18, 19) \
		M(b_m2b_1,        16, 17) \
		M(force_blend,    14, 14) \
		M(alpha_cvg_sel,  13, 13) \
		M(cvg_times_alpha,12, 12) \
		M(z_mode,         10, 11) \
		M(cvg_dest,        8,  9) \
		M(color_on_cvg,    7,  7) \
		M(image_read_en,   6,  6) \
		M(z_update_en,     5,  5) \
		M(z_compare_en,    4,  4) \
		M(antialias_en,    3,  3) \
		M(z_source_sel,    2,  2) \
		M(dither_alpha_en, 1,  1) \
		M(alpha_compare_en,0,  0)

	DEFINE_REGISTER_LAYOUT(RDP_SetOtherModes_Register, U64, RDP_SET_OTHER_MODES_FIELDS)

	// --- 0x30: Load TLUT ---
	#define RDP_LOAD_TLUT_FIELDS(M) \
		M(cmd,       56, 61) \
		M(SL,        44, 55) \
		M(TL,        32, 43) \
		M(tile,      24, 26) \
		M(SH,        12, 23) \
		M(TH,         0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_LoadTLUT_Register, U64, RDP_LOAD_TLUT_FIELDS)

	// --- 0x32: Set Tile Size ---
	#define RDP_SET_TILE_SIZE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(SL,        44, 55) \
		M(TL,        32, 43) \
		M(tile,      24, 26) \
		M(SH,        12, 23) \
		M(TH,         0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_SetTileSize_Register, U64, RDP_SET_TILE_SIZE_FIELDS)

	// --- 0x33: Load Block ---
	#define RDP_LOAD_BLOCK_FIELDS(M) \
		M(cmd,       56, 61) \
		M(SL,        44, 55) \
		M(TL,        32, 43) \
		M(tile,      24, 26) \
		M(SH,        12, 23) \
		M(dxt,        0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_LoadBlock_Register, U64, RDP_LOAD_BLOCK_FIELDS)

	// --- 0x34: Load Tile ---
	#define RDP_LOAD_TILE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(SL,        44, 55) \
		M(TL,        32, 43) \
		M(tile,      24, 26) \
		M(SH,        12, 23) \
		M(TH,         0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_LoadTile_Register, U64, RDP_LOAD_TILE_FIELDS)

	// --- 0x35: Set Tile ---
	#define RDP_SET_TILE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(format,    53, 55) \
		M(size,      51, 52) \
		M(line,      41, 49) \
		M(tmem_addr, 32, 40) \
		M(tile,      24, 26) \
		M(palette,   20, 23) \
		M(clamp_t,   19, 19) \
		M(mirror_t,  18, 18) \
		M(mask_t,    14, 17) \
		M(shift_t,   10, 13) \
		M(clamp_s,    9,  9) \
		M(mirror_s,   8,  8) \
		M(mask_s,     4,  7) \
		M(shift_s,    0,  3)

	DEFINE_REGISTER_LAYOUT(RDP_SetTile_Register, U64, RDP_SET_TILE_FIELDS)

	// --- 0x36: Fill Rectangle ---
	#define RDP_FILL_RECT_FIELDS(M) \
		M(cmd,       56, 61) \
		M(XL,        44, 55) \
		M(YL,        32, 43) \
		M(XH,        12, 23) \
		M(YH,         0, 11)

	DEFINE_REGISTER_LAYOUT(RDP_FillRect_Register, U64, RDP_FILL_RECT_FIELDS)

	// --- 0x37: Set Fill Color ---
	#define RDP_SET_FILL_COLOR_FIELDS(M) \
		M(cmd,       56, 61) \
		M(color,      0, 31)

	DEFINE_REGISTER_LAYOUT(RDP_SetFillColor_Register, U64, RDP_SET_FILL_COLOR_FIELDS)

	// --- 0x38: Set Fog Color ---
	#define RDP_SET_FOG_COLOR_FIELDS(M) \
		M(cmd,       56, 61) \
		M(R,         24, 31) \
		M(G,         16, 23) \
		M(B,          8, 15) \
		M(A,          0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetFogColor_Register, U64, RDP_SET_FOG_COLOR_FIELDS)

	// --- 0x39: Set Blend Color ---
	#define RDP_SET_BLEND_COLOR_FIELDS(M) \
		M(cmd,       56, 61) \
		M(R,         24, 31) \
		M(G,         16, 23) \
		M(B,          8, 15) \
		M(A,          0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetBlendColor_Register, U64, RDP_SET_BLEND_COLOR_FIELDS)

	// --- 0x3A: Set Primitive Color ---
	#define RDP_SET_PRIM_COLOR_FIELDS(M) \
		M(cmd,            56, 61) \
		M(min_level,      40, 44) \
		M(prim_lod_frac,  32, 39) \
		M(R,              24, 31) \
		M(G,              16, 23) \
		M(B,               8, 15) \
		M(A,               0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetPrimColor_Register, U64, RDP_SET_PRIM_COLOR_FIELDS)

	// --- 0x3B: Set Environment Color ---
	#define RDP_SET_ENV_COLOR_FIELDS(M) \
		M(cmd,       56, 61) \
		M(R,         24, 31) \
		M(G,         16, 23) \
		M(B,          8, 15) \
		M(A,          0,  7)

	DEFINE_REGISTER_LAYOUT(RDP_SetEnvColor_Register, U64, RDP_SET_ENV_COLOR_FIELDS)

	// --- 0x3C: Set Combine Mode ---
	#define RDP_SET_COMBINE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(sub_a_R_0, 52, 55) \
		M(mul_R_0,   47, 51) \
		M(sub_a_A_0, 44, 46) \
		M(mul_A_0,   41, 43) \
		M(sub_a_R_1, 37, 40) \
		M(mul_R_1,   32, 36) \
		M(sub_b_R_0, 28, 31) \
		M(sub_b_R_1, 24, 27) \
		M(sub_a_A_1, 21, 23) \
		M(mul_A_1,   18, 20) \
		M(add_R_0,   15, 17) \
		M(sub_b_A_0, 12, 14) \
		M(add_A_0,    9, 11) \
		M(add_R_1,    6,  8) \
		M(sub_b_A_1,  3,  5) \
		M(add_A_1,    0,  2)

	DEFINE_REGISTER_LAYOUT(RDP_SetCombine_Register, U64, RDP_SET_COMBINE_FIELDS)

	// --- 0x3D: Set Texture Image ---
	#define RDP_SET_TEX_IMAGE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(format,    53, 55) \
		M(size,      51, 52) \
		M(width,     32, 41) \
		M(address,    0, 25)

	DEFINE_REGISTER_LAYOUT(RDP_SetTexImage_Register, U64, RDP_SET_TEX_IMAGE_FIELDS)

	// --- 0x3E: Set Depth Image ---
	#define RDP_SET_DEPTH_IMAGE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(address,    0, 25)

	DEFINE_REGISTER_LAYOUT(RDP_SetDepthImage_Register, U64, RDP_SET_DEPTH_IMAGE_FIELDS)

	// --- 0x3F: Set Color Image ---
	#define RDP_SET_COLOR_IMAGE_FIELDS(M) \
		M(cmd,       56, 61) \
		M(format,    53, 55) \
		M(size,      51, 52) \
		M(width,     32, 41) \
		M(address,    0, 25)

	DEFINE_REGISTER_LAYOUT(RDP_SetColorImage_Register, U64, RDP_SET_COLOR_IMAGE_FIELDS)

	// =========================================================================
	//  Triangle shade/texture/depth coefficient registers (optional words)
	//  These are appended after the base triangle command depending on bits
	//  in the opcode (shade=bit2, texture=bit1, zbuffer=bit0 of cmd[3:1])
	// =========================================================================

	// Shade coefficients: 8 words (4 dwords)
	#define RDP_TRI_SHADE0_FIELDS(M) \
		M(R,         48, 63) \
		M(G,         32, 47) \
		M(B,         16, 31) \
		M(A,          0, 15)

	#define RDP_TRI_SHADE1_FIELDS(M) \
		M(DRDx,      48, 63) \
		M(DGDx,      32, 47) \
		M(DBDx,      16, 31) \
		M(DADx,       0, 15)

	#define RDP_TRI_SHADE2_FIELDS(M) \
		M(R_frac,    48, 63) \
		M(G_frac,    32, 47) \
		M(B_frac,    16, 31) \
		M(A_frac,     0, 15)

	#define RDP_TRI_SHADE3_FIELDS(M) \
		M(DRDx_frac, 48, 63) \
		M(DGDx_frac, 32, 47) \
		M(DBDx_frac, 16, 31) \
		M(DADx_frac,  0, 15)

	DEFINE_REGISTER_LAYOUT(RDP_Tri_Shade0_Register, U64, RDP_TRI_SHADE0_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Shade1_Register, U64, RDP_TRI_SHADE1_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Shade2_Register, U64, RDP_TRI_SHADE2_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Shade3_Register, U64, RDP_TRI_SHADE3_FIELDS)

	// Texture coefficients: 8 words (4 dwords)
	#define RDP_TRI_TEX0_FIELDS(M) \
		M(S,         48, 63) \
		M(T,         32, 47) \
		M(W,         16, 31)

	#define RDP_TRI_TEX1_FIELDS(M) \
		M(DSDx,      48, 63) \
		M(DTDx,      32, 47) \
		M(DWDx,      16, 31)

	#define RDP_TRI_TEX2_FIELDS(M) \
		M(S_frac,    48, 63) \
		M(T_frac,    32, 47) \
		M(W_frac,    16, 31)

	#define RDP_TRI_TEX3_FIELDS(M) \
		M(DSDx_frac, 48, 63) \
		M(DTDx_frac, 32, 47) \
		M(DWDx_frac, 16, 31)

	DEFINE_REGISTER_LAYOUT(RDP_Tri_Tex0_Register, U64, RDP_TRI_TEX0_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Tex1_Register, U64, RDP_TRI_TEX1_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Tex2_Register, U64, RDP_TRI_TEX2_FIELDS)
	DEFINE_REGISTER_LAYOUT(RDP_Tri_Tex3_Register, U64, RDP_TRI_TEX3_FIELDS)

	// Depth coefficients: 2 words (1 dword)
	#define RDP_TRI_DEPTH_FIELDS(M) \
		M(Z,         48, 63) \
		M(DZDx,      32, 47) \
		M(Z_frac,    16, 31) \
		M(DZDx_frac,  0, 15)

	DEFINE_REGISTER_LAYOUT(RDP_Tri_Depth_Register, U64, RDP_TRI_DEPTH_FIELDS)

	// =========================================================================
	//  Triangle command data structure (all optional sections)
	// =========================================================================
	struct RDPTriangleData {
		RDP_Tri_Word0_Register word0;
		RDP_Tri_Word1_Register word1;
		RDP_Tri_Word2_Register word2;
		RDP_Tri_Word3_Register word3;

		// Optional shade (present if opcode bit 2 set)
		BIT hasShade = ESX_FALSE;
		RDP_Tri_Shade0_Register shade0;
		RDP_Tri_Shade1_Register shade1;
		RDP_Tri_Shade2_Register shade2;
		RDP_Tri_Shade3_Register shade3;

		// Optional texture (present if opcode bit 1 set)
		BIT hasTexture = ESX_FALSE;
		RDP_Tri_Tex0_Register tex0;
		RDP_Tri_Tex1_Register tex1;
		RDP_Tri_Tex2_Register tex2;
		RDP_Tri_Tex3_Register tex3;

		// Optional depth (present if opcode bit 0 set)
		BIT hasDepth = ESX_FALSE;
		RDP_Tri_Depth_Register depth;
	};

	// =========================================================================
	//  RDP Class
	// =========================================================================
	class RDP {
		friend class CPUStatusPanel;
	public:
		RDP(RCP* rcp);
		~RDP();

		void clock(U64 clocks);
		void init();
		void reset();

		// Main dispatch: returns number of 64-bit words consumed
		U32 executeCommand(const U64* commandStream, U32 numDWords);

		// --- Command handlers (to be implemented) ---
		void FillTriangle(const RDPTriangleData& tri);
		void TextureRectangle(const RDP_TexRect_Word0_Register& w0, const RDP_TexRect_Word1_Register& w1);
		void TextureRectangleFlip(const RDP_TexRect_Word0_Register& w0, const RDP_TexRect_Word1_Register& w1);
		void SyncLoad();
		void SyncPipe();
		void SyncTile();
		void SyncFull();
		void SetKeyGB(const RDP_SetKeyGB_Register& reg);
		void SetKeyR(const RDP_SetKeyR_Register& reg);
		void SetConvert(const RDP_SetConvert_Register& reg);
		void SetScissor(const RDP_SetScissor_Register& reg);
		void SetPrimDepth(const RDP_SetPrimDepth_Register& reg);
		void SetOtherModes(const RDP_SetOtherModes_Register& reg);
		void LoadTLUT(const RDP_LoadTLUT_Register& reg);
		void SetTileSize(const RDP_SetTileSize_Register& reg);
		void LoadBlock(const RDP_LoadBlock_Register& reg);
		void LoadTile(const RDP_LoadTile_Register& reg);
		void SetTile(const RDP_SetTile_Register& reg);
		void FillRectangle(const RDP_FillRect_Register& reg);
		void SetFillColor(const RDP_SetFillColor_Register& reg);
		void SetFogColor(const RDP_SetFogColor_Register& reg);
		void SetBlendColor(const RDP_SetBlendColor_Register& reg);
		void SetPrimColor(const RDP_SetPrimColor_Register& reg);
		void SetEnvColor(const RDP_SetEnvColor_Register& reg);
		void SetCombineMode(const RDP_SetCombine_Register& reg);
		void SetTextureImage(const RDP_SetTexImage_Register& reg);
		void SetDepthImage(const RDP_SetDepthImage_Register& reg);
		void SetColorImage(const RDP_SetColorImage_Register& reg);

	private:
		StringView mName = "RDP";
		RCP* mRCP;
	};

}