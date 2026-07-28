#include "RDP.h"

#include "../RCP.h"

namespace esx {

	RDP::RDP(RCP* rcp)
		: mRCP(rcp)
	{
	}

	RDP::~RDP()
	{
	}

	void RDP::clock(U64 clocks)
	{
	}

	void RDP::init()
	{
	}

	void RDP::reset()
	{
	}

	U32 RDP::executeCommand(const U64* commandStream, U32 numDWords)
	{
		if (numDWords == 0) return 0;

		U64 firstWord = commandStream[0];
		U8 opcode = (firstWord >> 56) & 0x3F;

		switch (opcode) {
			// --- No Operation ---
			case 0x00: case 0x01: case 0x02: case 0x03:
			case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x10: case 0x11: case 0x12: case 0x13:
			case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1A: case 0x1B:
			case 0x1C: case 0x1D: case 0x1E: case 0x1F:
			case 0x20: case 0x21: case 0x22: case 0x23:
			case 0x31:
				return 1;

			// --- Fill Triangle (0x08-0x0F) ---
			case 0x08: case 0x09: case 0x0A: case 0x0B:
			case 0x0C: case 0x0D: case 0x0E: case 0x0F: {
				RDPTriangleData tri;
				tri.word0.write(commandStream[0]);

				BIT hasShade   = tri.word0.get<layouts::RDP_Tri_Word0_Register::shade>();
				BIT hasTexture = tri.word0.get<layouts::RDP_Tri_Word0_Register::texture>();
				BIT hasDepth   = tri.word0.get<layouts::RDP_Tri_Word0_Register::zbuffer>();

				U32 wordsNeeded = 4; // base triangle = 4 dwords
				if (hasShade)   wordsNeeded += 4;
				if (hasTexture) wordsNeeded += 4;
				if (hasDepth)   wordsNeeded += 1;

				if (numDWords < wordsNeeded) return 0; // not enough data

				tri.word1.write(commandStream[1]);
				tri.word2.write(commandStream[2]);
				tri.word3.write(commandStream[3]);

				U32 idx = 4;
				if (hasShade) {
					tri.hasShade = ESX_TRUE;
					tri.shade0.write(commandStream[idx++]);
					tri.shade1.write(commandStream[idx++]);
					tri.shade2.write(commandStream[idx++]);
					tri.shade3.write(commandStream[idx++]);
				}
				if (hasTexture) {
					tri.hasTexture = ESX_TRUE;
					tri.tex0.write(commandStream[idx++]);
					tri.tex1.write(commandStream[idx++]);
					tri.tex2.write(commandStream[idx++]);
					tri.tex3.write(commandStream[idx++]);
				}
				if (hasDepth) {
					tri.hasDepth = ESX_TRUE;
					tri.depth.write(commandStream[idx++]);
				}

				FillTriangle(tri);
				return wordsNeeded;
			}

			// --- Texture Rectangle (0x24) ---
			case 0x24: {
				if (numDWords < 2) return 0;
				RDP_TexRect_Word0_Register w0;
				RDP_TexRect_Word1_Register w1;
				w0.write(commandStream[0]);
				w1.write(commandStream[1]);
				TextureRectangle(w0, w1);
				return 2;
			}

			// --- Texture Rectangle Flip (0x25) ---
			case 0x25: {
				if (numDWords < 2) return 0;
				RDP_TexRect_Word0_Register w0;
				RDP_TexRect_Word1_Register w1;
				w0.write(commandStream[0]);
				w1.write(commandStream[1]);
				TextureRectangleFlip(w0, w1);
				return 2;
			}

			// --- Sync Load (0x26) ---
			case 0x26: SyncLoad(); return 1;

			// --- Sync Pipe (0x27) ---
			case 0x27: SyncPipe(); return 1;

			// --- Sync Tile (0x28) ---
			case 0x28: SyncTile(); return 1;

			// --- Sync Full (0x29) ---
			case 0x29: SyncFull(); return 1;

			// --- Set Key GB (0x2A) ---
			case 0x2A: {
				RDP_SetKeyGB_Register reg;
				reg.write(firstWord);
				SetKeyGB(reg);
				return 1;
			}

			// --- Set Key R (0x2B) ---
			case 0x2B: {
				RDP_SetKeyR_Register reg;
				reg.write(firstWord);
				SetKeyR(reg);
				return 1;
			}

			// --- Set Convert (0x2C) ---
			case 0x2C: {
				RDP_SetConvert_Register reg;
				reg.write(firstWord);
				SetConvert(reg);
				return 1;
			}

			// --- Set Scissor (0x2D) ---
			case 0x2D: {
				RDP_SetScissor_Register reg;
				reg.write(firstWord);
				SetScissor(reg);
				return 1;
			}

			// --- Set Primitive Depth (0x2E) ---
			case 0x2E: {
				RDP_SetPrimDepth_Register reg;
				reg.write(firstWord);
				SetPrimDepth(reg);
				return 1;
			}

			// --- Set Other Modes (0x2F) ---
			case 0x2F: {
				RDP_SetOtherModes_Register reg;
				reg.write(firstWord);
				SetOtherModes(reg);
				return 1;
			}

			// --- Load TLUT (0x30) ---
			case 0x30: {
				RDP_LoadTLUT_Register reg;
				reg.write(firstWord);
				LoadTLUT(reg);
				return 1;
			}

			// --- Set Tile Size (0x32) ---
			case 0x32: {
				RDP_SetTileSize_Register reg;
				reg.write(firstWord);
				SetTileSize(reg);
				return 1;
			}

			// --- Load Block (0x33) ---
			case 0x33: {
				RDP_LoadBlock_Register reg;
				reg.write(firstWord);
				LoadBlock(reg);
				return 1;
			}

			// --- Load Tile (0x34) ---
			case 0x34: {
				RDP_LoadTile_Register reg;
				reg.write(firstWord);
				LoadTile(reg);
				return 1;
			}

			// --- Set Tile (0x35) ---
			case 0x35: {
				RDP_SetTile_Register reg;
				reg.write(firstWord);
				SetTile(reg);
				return 1;
			}

			// --- Fill Rectangle (0x36) ---
			case 0x36: {
				RDP_FillRect_Register reg;
				reg.write(firstWord);
				FillRectangle(reg);
				return 1;
			}

			// --- Set Fill Color (0x37) ---
			case 0x37: {
				RDP_SetFillColor_Register reg;
				reg.write(firstWord);
				SetFillColor(reg);
				return 1;
			}

			// --- Set Fog Color (0x38) ---
			case 0x38: {
				RDP_SetFogColor_Register reg;
				reg.write(firstWord);
				SetFogColor(reg);
				return 1;
			}

			// --- Set Blend Color (0x39) ---
			case 0x39: {
				RDP_SetBlendColor_Register reg;
				reg.write(firstWord);
				SetBlendColor(reg);
				return 1;
			}

			// --- Set Primitive Color (0x3A) ---
			case 0x3A: {
				RDP_SetPrimColor_Register reg;
				reg.write(firstWord);
				SetPrimColor(reg);
				return 1;
			}

			// --- Set Environment Color (0x3B) ---
			case 0x3B: {
				RDP_SetEnvColor_Register reg;
				reg.write(firstWord);
				SetEnvColor(reg);
				return 1;
			}

			// --- Set Combine Mode (0x3C) ---
			case 0x3C: {
				RDP_SetCombine_Register reg;
				reg.write(firstWord);
				SetCombineMode(reg);
				return 1;
			}

			// --- Set Texture Image (0x3D) ---
			case 0x3D: {
				RDP_SetTexImage_Register reg;
				reg.write(firstWord);
				SetTextureImage(reg);
				return 1;
			}

			// --- Set Depth Image (0x3E) ---
			case 0x3E: {
				RDP_SetDepthImage_Register reg;
				reg.write(firstWord);
				SetDepthImage(reg);
				return 1;
			}

			// --- Set Color Image (0x3F) ---
			case 0x3F: {
				RDP_SetColorImage_Register reg;
				reg.write(firstWord);
				SetColorImage(reg);
				return 1;
			}

			default:
				ESX_CORE_LOG_WARNING("RDP - Unknown command opcode: 0x{:02x}", opcode);
				return 1;
		}
	}

	// =========================================================================
	//  Command handler stubs
	// =========================================================================

	void RDP::FillTriangle(const RDPTriangleData& tri)
	{
		ESX_CORE_LOG_WARNING("RDP::FillTriangle - Not implemented yet | shade={} texture={} zbuffer={} lft={} level={} tile={} YL={} YM={} YH={}",
			tri.word0.get<layouts::RDP_Tri_Word0_Register::shade>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::texture>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::zbuffer>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::lft>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::level>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::tile>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::YL>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::YM>(),
			tri.word0.get<layouts::RDP_Tri_Word0_Register::YH>());
	}

	void RDP::TextureRectangle(const RDP_TexRect_Word0_Register& w0, const RDP_TexRect_Word1_Register& w1)
	{
		ESX_CORE_LOG_WARNING("RDP::TextureRectangle - Not implemented yet | XL={} YL={} tile={} XH={} YH={} S={} T={} DsDx={} DtDy={}",
			w0.get<layouts::RDP_TexRect_Word0_Register::XL>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::YL>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::tile>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::XH>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::YH>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::S>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::T>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::DsDx>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::DtDy>());
	}

	void RDP::TextureRectangleFlip(const RDP_TexRect_Word0_Register& w0, const RDP_TexRect_Word1_Register& w1)
	{
		ESX_CORE_LOG_WARNING("RDP::TextureRectangleFlip - Not implemented yet | XL={} YL={} tile={} XH={} YH={} S={} T={} DsDx={} DtDy={}",
			w0.get<layouts::RDP_TexRect_Word0_Register::XL>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::YL>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::tile>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::XH>(),
			w0.get<layouts::RDP_TexRect_Word0_Register::YH>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::S>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::T>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::DsDx>(),
			w1.get<layouts::RDP_TexRect_Word1_Register::DtDy>());
	}

	void RDP::SyncLoad()
	{
		ESX_CORE_LOG_WARNING("RDP::SyncLoad - Not implemented yet");
	}

	void RDP::SyncPipe()
	{
		ESX_CORE_LOG_WARNING("RDP::SyncPipe - Not implemented yet");
	}

	void RDP::SyncTile()
	{
		ESX_CORE_LOG_WARNING("RDP::SyncTile - Not implemented yet");
	}

	void RDP::SyncFull()
	{
		ESX_CORE_LOG_WARNING("RDP::SyncFull - Not implemented yet");
		mRCP->setInterrupt(InterruptType::DP, ESX_FALSE, ESX_TRUE, 0);
	}

	void RDP::SetKeyGB(const RDP_SetKeyGB_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetKeyGB - Not implemented yet | width_g={} width_b={} center_g={} scale_g={} center_b={} scale_b={}",
			reg.get<layouts::RDP_SetKeyGB_Register::width_g>(),
			reg.get<layouts::RDP_SetKeyGB_Register::width_b>(),
			reg.get<layouts::RDP_SetKeyGB_Register::center_g>(),
			reg.get<layouts::RDP_SetKeyGB_Register::scale_g>(),
			reg.get<layouts::RDP_SetKeyGB_Register::center_b>(),
			reg.get<layouts::RDP_SetKeyGB_Register::scale_b>());
	}

	void RDP::SetKeyR(const RDP_SetKeyR_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetKeyR - Not implemented yet | width_r={} center_r={} scale_r={}",
			reg.get<layouts::RDP_SetKeyR_Register::width_r>(),
			reg.get<layouts::RDP_SetKeyR_Register::center_r>(),
			reg.get<layouts::RDP_SetKeyR_Register::scale_r>());
	}

	void RDP::SetConvert(const RDP_SetConvert_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetConvert - Not implemented yet | k0={} k1={} k2={} k3={} k4={} k5={}",
			reg.get<layouts::RDP_SetConvert_Register::k0>(),
			reg.get<layouts::RDP_SetConvert_Register::k1>(),
			reg.get<layouts::RDP_SetConvert_Register::k2>(),
			reg.get<layouts::RDP_SetConvert_Register::k3>(),
			reg.get<layouts::RDP_SetConvert_Register::k4>(),
			reg.get<layouts::RDP_SetConvert_Register::k5>());
	}

	void RDP::SetScissor(const RDP_SetScissor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetScissor - Not implemented yet | XH={} YH={} f={} o={} XL={} YL={}",
			reg.get<layouts::RDP_SetScissor_Register::XH>(),
			reg.get<layouts::RDP_SetScissor_Register::YH>(),
			reg.get<layouts::RDP_SetScissor_Register::f>(),
			reg.get<layouts::RDP_SetScissor_Register::o>(),
			reg.get<layouts::RDP_SetScissor_Register::XL>(),
			reg.get<layouts::RDP_SetScissor_Register::YL>());
	}

	void RDP::SetPrimDepth(const RDP_SetPrimDepth_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetPrimDepth - Not implemented yet | z={} dz={}",
			reg.get<layouts::RDP_SetPrimDepth_Register::z>(),
			reg.get<layouts::RDP_SetPrimDepth_Register::dz>());
	}

	void RDP::SetOtherModes(const RDP_SetOtherModes_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetOtherModes - Not implemented yet | cycle_type={} persp_tex_en={} z_compare_en={} z_update_en={} antialias_en={} raw={:016x}",
			reg.get<layouts::RDP_SetOtherModes_Register::cycle_type>(),
			reg.get<layouts::RDP_SetOtherModes_Register::persp_tex_en>(),
			reg.get<layouts::RDP_SetOtherModes_Register::z_compare_en>(),
			reg.get<layouts::RDP_SetOtherModes_Register::z_update_en>(),
			reg.get<layouts::RDP_SetOtherModes_Register::antialias_en>(),
			reg.read());
	}

	void RDP::LoadTLUT(const RDP_LoadTLUT_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::LoadTLUT - Not implemented yet | SL={} TL={} tile={} SH={} TH={}",
			reg.get<layouts::RDP_LoadTLUT_Register::SL>(),
			reg.get<layouts::RDP_LoadTLUT_Register::TL>(),
			reg.get<layouts::RDP_LoadTLUT_Register::tile>(),
			reg.get<layouts::RDP_LoadTLUT_Register::SH>(),
			reg.get<layouts::RDP_LoadTLUT_Register::TH>());
	}

	void RDP::SetTileSize(const RDP_SetTileSize_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetTileSize - Not implemented yet | SL={} TL={} tile={} SH={} TH={}",
			reg.get<layouts::RDP_SetTileSize_Register::SL>(),
			reg.get<layouts::RDP_SetTileSize_Register::TL>(),
			reg.get<layouts::RDP_SetTileSize_Register::tile>(),
			reg.get<layouts::RDP_SetTileSize_Register::SH>(),
			reg.get<layouts::RDP_SetTileSize_Register::TH>());
	}

	void RDP::LoadBlock(const RDP_LoadBlock_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::LoadBlock - Not implemented yet | SL={} TL={} tile={} SH={} dxt={}",
			reg.get<layouts::RDP_LoadBlock_Register::SL>(),
			reg.get<layouts::RDP_LoadBlock_Register::TL>(),
			reg.get<layouts::RDP_LoadBlock_Register::tile>(),
			reg.get<layouts::RDP_LoadBlock_Register::SH>(),
			reg.get<layouts::RDP_LoadBlock_Register::dxt>());
	}

	void RDP::LoadTile(const RDP_LoadTile_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::LoadTile - Not implemented yet | SL={} TL={} tile={} SH={} TH={}",
			reg.get<layouts::RDP_LoadTile_Register::SL>(),
			reg.get<layouts::RDP_LoadTile_Register::TL>(),
			reg.get<layouts::RDP_LoadTile_Register::tile>(),
			reg.get<layouts::RDP_LoadTile_Register::SH>(),
			reg.get<layouts::RDP_LoadTile_Register::TH>());
	}

	void RDP::SetTile(const RDP_SetTile_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetTile - Not implemented yet | format={} size={} line={} tmem_addr={} tile={} palette={} clamp_t={} mirror_t={} mask_t={} shift_t={} clamp_s={} mirror_s={} mask_s={} shift_s={}",
			reg.get<layouts::RDP_SetTile_Register::format>(),
			reg.get<layouts::RDP_SetTile_Register::size>(),
			reg.get<layouts::RDP_SetTile_Register::line>(),
			reg.get<layouts::RDP_SetTile_Register::tmem_addr>(),
			reg.get<layouts::RDP_SetTile_Register::tile>(),
			reg.get<layouts::RDP_SetTile_Register::palette>(),
			reg.get<layouts::RDP_SetTile_Register::clamp_t>(),
			reg.get<layouts::RDP_SetTile_Register::mirror_t>(),
			reg.get<layouts::RDP_SetTile_Register::mask_t>(),
			reg.get<layouts::RDP_SetTile_Register::shift_t>(),
			reg.get<layouts::RDP_SetTile_Register::clamp_s>(),
			reg.get<layouts::RDP_SetTile_Register::mirror_s>(),
			reg.get<layouts::RDP_SetTile_Register::mask_s>(),
			reg.get<layouts::RDP_SetTile_Register::shift_s>());
	}

	void RDP::FillRectangle(const RDP_FillRect_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::FillRectangle - Not implemented yet | XL={} YL={} XH={} YH={}",
			reg.get<layouts::RDP_FillRect_Register::XL>(),
			reg.get<layouts::RDP_FillRect_Register::YL>(),
			reg.get<layouts::RDP_FillRect_Register::XH>(),
			reg.get<layouts::RDP_FillRect_Register::YH>());
	}

	void RDP::SetFillColor(const RDP_SetFillColor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetFillColor - Not implemented yet | color={:08x}",
			reg.get<layouts::RDP_SetFillColor_Register::color>());
	}

	void RDP::SetFogColor(const RDP_SetFogColor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetFogColor - Not implemented yet | R={} G={} B={} A={}",
			reg.get<layouts::RDP_SetFogColor_Register::R>(),
			reg.get<layouts::RDP_SetFogColor_Register::G>(),
			reg.get<layouts::RDP_SetFogColor_Register::B>(),
			reg.get<layouts::RDP_SetFogColor_Register::A>());
	}

	void RDP::SetBlendColor(const RDP_SetBlendColor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetBlendColor - Not implemented yet | R={} G={} B={} A={}",
			reg.get<layouts::RDP_SetBlendColor_Register::R>(),
			reg.get<layouts::RDP_SetBlendColor_Register::G>(),
			reg.get<layouts::RDP_SetBlendColor_Register::B>(),
			reg.get<layouts::RDP_SetBlendColor_Register::A>());
	}

	void RDP::SetPrimColor(const RDP_SetPrimColor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetPrimColor - Not implemented yet | min_level={} prim_lod_frac={} R={} G={} B={} A={}",
			reg.get<layouts::RDP_SetPrimColor_Register::min_level>(),
			reg.get<layouts::RDP_SetPrimColor_Register::prim_lod_frac>(),
			reg.get<layouts::RDP_SetPrimColor_Register::R>(),
			reg.get<layouts::RDP_SetPrimColor_Register::G>(),
			reg.get<layouts::RDP_SetPrimColor_Register::B>(),
			reg.get<layouts::RDP_SetPrimColor_Register::A>());
	}

	void RDP::SetEnvColor(const RDP_SetEnvColor_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetEnvColor - Not implemented yet | R={} G={} B={} A={}",
			reg.get<layouts::RDP_SetEnvColor_Register::R>(),
			reg.get<layouts::RDP_SetEnvColor_Register::G>(),
			reg.get<layouts::RDP_SetEnvColor_Register::B>(),
			reg.get<layouts::RDP_SetEnvColor_Register::A>());
	}

	void RDP::SetCombineMode(const RDP_SetCombine_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetCombineMode - Not implemented yet | sub_a_R_0={} mul_R_0={} sub_a_A_0={} mul_A_0={} sub_a_R_1={} mul_R_1={} sub_b_R_0={} sub_b_R_1={} add_R_0={} add_R_1={} raw={:016x}",
			reg.get<layouts::RDP_SetCombine_Register::sub_a_R_0>(),
			reg.get<layouts::RDP_SetCombine_Register::mul_R_0>(),
			reg.get<layouts::RDP_SetCombine_Register::sub_a_A_0>(),
			reg.get<layouts::RDP_SetCombine_Register::mul_A_0>(),
			reg.get<layouts::RDP_SetCombine_Register::sub_a_R_1>(),
			reg.get<layouts::RDP_SetCombine_Register::mul_R_1>(),
			reg.get<layouts::RDP_SetCombine_Register::sub_b_R_0>(),
			reg.get<layouts::RDP_SetCombine_Register::sub_b_R_1>(),
			reg.get<layouts::RDP_SetCombine_Register::add_R_0>(),
			reg.get<layouts::RDP_SetCombine_Register::add_R_1>(),
			reg.read());
	}

	void RDP::SetTextureImage(const RDP_SetTexImage_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetTextureImage - Not implemented yet | format={} size={} width={} address={:06x}",
			reg.get<layouts::RDP_SetTexImage_Register::format>(),
			reg.get<layouts::RDP_SetTexImage_Register::size>(),
			reg.get<layouts::RDP_SetTexImage_Register::width>(),
			reg.get<layouts::RDP_SetTexImage_Register::address>());
	}

	void RDP::SetDepthImage(const RDP_SetDepthImage_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetDepthImage - Not implemented yet | address={:06x}",
			reg.get<layouts::RDP_SetDepthImage_Register::address>());
	}

	void RDP::SetColorImage(const RDP_SetColorImage_Register& reg)
	{
		ESX_CORE_LOG_WARNING("RDP::SetColorImage - Not implemented yet | format={} size={} width={} address={:06x}",
			reg.get<layouts::RDP_SetColorImage_Register::format>(),
			reg.get<layouts::RDP_SetColorImage_Register::size>(),
			reg.get<layouts::RDP_SetColorImage_Register::width>(),
			reg.get<layouts::RDP_SetColorImage_Register::address>());
	}

}
