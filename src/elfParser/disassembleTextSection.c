#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"

#include "dataOperate.h"

extern uint64_t shdrTextOff;
extern uint64_t shdrTextSize;
extern uint8_t riscvLen;

static uint8_t compressionInstLen = 2;
static uint8_t uncompressionInstLen = 4;

/* types */

typedef uint64_t rv_inst;
typedef uint16_t rv_opcode;

/* enums */

typedef enum { rv32, rv64, rv128 } rv_isa;

typedef enum {
  rv_rm_rne = 0,
  rv_rm_rtz = 1,
  rv_rm_rdn = 2,
  rv_rm_rup = 3,
  rv_rm_rmm = 4,
  rv_rm_dyn = 7,
} rv_rm;

typedef enum {
  rv_fence_i = 8,
  rv_fence_o = 4,
  rv_fence_r = 2,
  rv_fence_w = 1,
} rv_fence;

typedef enum {
  reg_x00_f00,
  reg_x01_f01,
  reg_x02_f02,
  reg_x03_f03,
  reg_x04_f04,
  reg_x05_f05,
  reg_x06_f06,
  reg_x07_f07,
  reg_x08_f08,
  reg_x09_f09,
  reg_x10_f10,
  reg_x11_f11,
  reg_x12_f12,
  reg_x13_f13,
  reg_x14_f14,
  reg_x15_f15,
  reg_x16_f16,
  reg_x17_f17,
  reg_x18_f18,
  reg_x19_f19,
  reg_x20_f20,
  reg_x21_f21,
  reg_x22_f22,
  reg_x23_f23,
  reg_x24_f24,
  reg_x25_f25,
  reg_x26_f26,
  reg_x27_f27,
  reg_x28_f28,
  reg_x29_f29,
  reg_x30_f30,
  reg_x31_f31,
} rv_reg;

typedef enum {
  rvc_end,
  rvc_rd_eq_ra,
  rvc_rd_eq_x0,
  rvc_rs1_eq_x0,
  rvc_rs2_eq_x0,
  rvc_rs2_eq_rs1,
  rvc_rs1_eq_ra,
  rvc_imm_eq_zero,
  rvc_imm_eq_n1,
  rvc_imm_eq_p1,
  rvc_csr_eq_0x001,
  rvc_csr_eq_0x002,
  rvc_csr_eq_0x003,
  rvc_csr_eq_0xc00,
  rvc_csr_eq_0xc01,
  rvc_csr_eq_0xc02,
  rvc_csr_eq_0xc80,
  rvc_csr_eq_0xc81,
  rvc_csr_eq_0xc82,
} rvc_constraint;

typedef enum {
  rv_encodingType_illegal,
  rv_encodingType_none,
  rv_encodingType_u,
  rv_encodingType_uj,
  rv_encodingType_i,
  rv_encodingType_i_sh5,
  rv_encodingType_i_sh6,
  rv_encodingType_i_sh7,
  rv_encodingType_i_csr,
  rv_encodingType_s,
  rv_encodingType_sb,
  rv_encodingType_r,
  rv_encodingType_r_m,
  rv_encodingType_r4_m,
  rv_encodingType_r_a,
  rv_encodingType_r_l,
  rv_encodingType_r_f,
  rv_encodingType_cb,
  rv_encodingType_cb_imm,
  rv_encodingType_cb_sh5,
  rv_encodingType_cb_sh6,
  rv_encodingType_ci,
  rv_encodingType_ci_sh5,
  rv_encodingType_ci_sh6,
  rv_encodingType_ci_16sp,
  rv_encodingType_ci_lwsp,
  rv_encodingType_ci_ldsp,
  rv_encodingType_ci_lqsp,
  rv_encodingType_ci_li,
  rv_encodingType_ci_lui,
  rv_encodingType_ci_none,
  rv_encodingType_ciw_4spn,
  rv_encodingType_cj,
  rv_encodingType_cj_jal,
  rv_encodingType_cl_lw,
  rv_encodingType_cl_ld,
  rv_encodingType_cl_lq,
  rv_encodingType_cr,
  rv_encodingType_cr_mv,
  rv_encodingType_cr_jalr,
  rv_encodingType_cr_jr,
  rv_encodingType_cs,
  rv_encodingType_cs_sw,
  rv_encodingType_cs_sd,
  rv_encodingType_cs_sq,
  rv_encodingType_css_swsp,
  rv_encodingType_css_sdsp,
  rv_encodingType_css_sqsp,
} rv_encodingType;

typedef enum {
  rv_op_illegal,
  rv_op_lui,
  rv_op_auipc,
  rv_op_jal,
  rv_op_jalr,
  rv_op_beq,
  rv_op_bne,
  rv_op_blt,
  rv_op_bge,
  rv_op_bltu,
  rv_op_bgeu,
  rv_op_lb,
  rv_op_lh,
  rv_op_lw,
  rv_op_lbu,
  rv_op_lhu,
  rv_op_sb,
  rv_op_sh,
  rv_op_sw,
  rv_op_addi,
  rv_op_slti,
  rv_op_sltiu,
  rv_op_xori,
  rv_op_ori,
  rv_op_andi,
  rv_op_slli,
  rv_op_srli,
  rv_op_srai,
  rv_op_add,
  rv_op_sub,
  rv_op_sll,
  rv_op_slt,
  rv_op_sltu,
  rv_op_xor,
  rv_op_srl,
  rv_op_sra,
  rv_op_or,
  rv_op_and,
  rv_op_fence,
  rv_op_fence_i,
  rv_op_lwu,
  rv_op_ld,
  rv_op_sd,
  rv_op_addiw,
  rv_op_slliw,
  rv_op_srliw,
  rv_op_sraiw,
  rv_op_addw,
  rv_op_subw,
  rv_op_sllw,
  rv_op_srlw,
  rv_op_sraw,
  rv_op_ldu,
  rv_op_lq,
  rv_op_sq,
  rv_op_addid,
  rv_op_sllid,
  rv_op_srlid,
  rv_op_sraid,
  rv_op_addd,
  rv_op_subd,
  rv_op_slld,
  rv_op_srld,
  rv_op_srad,
  rv_op_mul,
  rv_op_mulh,
  rv_op_mulhsu,
  rv_op_mulhu,
  rv_op_div,
  rv_op_divu,
  rv_op_rem,
  rv_op_remu,
  rv_op_mulw,
  rv_op_divw,
  rv_op_divuw,
  rv_op_remw,
  rv_op_remuw,
  rv_op_muld,
  rv_op_divd,
  rv_op_divud,
  rv_op_remd,
  rv_op_remud,
  rv_op_lr_w,
  rv_op_sc_w,
  rv_op_amoswap_w,
  rv_op_amoadd_w,
  rv_op_amoxor_w,
  rv_op_amoor_w,
  rv_op_amoand_w,
  rv_op_amomin_w,
  rv_op_amomax_w,
  rv_op_amominu_w,
  rv_op_amomaxu_w,
  rv_op_lr_d,
  rv_op_sc_d,
  rv_op_amoswap_d,
  rv_op_amoadd_d,
  rv_op_amoxor_d,
  rv_op_amoor_d,
  rv_op_amoand_d,
  rv_op_amomin_d,
  rv_op_amomax_d,
  rv_op_amominu_d,
  rv_op_amomaxu_d,
  rv_op_lr_q,
  rv_op_sc_q,
  rv_op_amoswap_q,
  rv_op_amoadd_q,
  rv_op_amoxor_q,
  rv_op_amoor_q,
  rv_op_amoand_q,
  rv_op_amomin_q,
  rv_op_amomax_q,
  rv_op_amominu_q,
  rv_op_amomaxu_q,
  rv_op_ecall,
  rv_op_ebreak,
  rv_op_uret,
  rv_op_sret,
  rv_op_hret,
  rv_op_mret,
  rv_op_dret,
  rv_op_sfence_vm,
  rv_op_sfence_vma,
  rv_op_wfi,
  rv_op_csrrw,
  rv_op_csrrs,
  rv_op_csrrc,
  rv_op_csrrwi,
  rv_op_csrrsi,
  rv_op_csrrci,
  rv_op_flw,
  rv_op_fsw,
  rv_op_fmadd_s,
  rv_op_fmsub_s,
  rv_op_fnmsub_s,
  rv_op_fnmadd_s,
  rv_op_fadd_s,
  rv_op_fsub_s,
  rv_op_fmul_s,
  rv_op_fdiv_s,
  rv_op_fsgnj_s,
  rv_op_fsgnjn_s,
  rv_op_fsgnjx_s,
  rv_op_fmin_s,
  rv_op_fmax_s,
  rv_op_fsqrt_s,
  rv_op_fle_s,
  rv_op_flt_s,
  rv_op_feq_s,
  rv_op_fcvt_w_s,
  rv_op_fcvt_wu_s,
  rv_op_fcvt_s_w,
  rv_op_fcvt_s_wu,
  rv_op_fmv_x_s,
  rv_op_fclass_s,
  rv_op_fmv_s_x,
  rv_op_fcvt_l_s,
  rv_op_fcvt_lu_s,
  rv_op_fcvt_s_l,
  rv_op_fcvt_s_lu,
  rv_op_fld,
  rv_op_fsd,
  rv_op_fmadd_d,
  rv_op_fmsub_d,
  rv_op_fnmsub_d,
  rv_op_fnmadd_d,
  rv_op_fadd_d,
  rv_op_fsub_d,
  rv_op_fmul_d,
  rv_op_fdiv_d,
  rv_op_fsgnj_d,
  rv_op_fsgnjn_d,
  rv_op_fsgnjx_d,
  rv_op_fmin_d,
  rv_op_fmax_d,
  rv_op_fcvt_s_d,
  rv_op_fcvt_d_s,
  rv_op_fsqrt_d,
  rv_op_fle_d,
  rv_op_flt_d,
  rv_op_feq_d,
  rv_op_fcvt_w_d,
  rv_op_fcvt_wu_d,
  rv_op_fcvt_d_w,
  rv_op_fcvt_d_wu,
  rv_op_fclass_d,
  rv_op_fcvt_l_d,
  rv_op_fcvt_lu_d,
  rv_op_fmv_x_d,
  rv_op_fcvt_d_l,
  rv_op_fcvt_d_lu,
  rv_op_fmv_d_x,
  rv_op_flq,
  rv_op_fsq,
  rv_op_fmadd_q,
  rv_op_fmsub_q,
  rv_op_fnmsub_q,
  rv_op_fnmadd_q,
  rv_op_fadd_q,
  rv_op_fsub_q,
  rv_op_fmul_q,
  rv_op_fdiv_q,
  rv_op_fsgnj_q,
  rv_op_fsgnjn_q,
  rv_op_fsgnjx_q,
  rv_op_fmin_q,
  rv_op_fmax_q,
  rv_op_fcvt_s_q,
  rv_op_fcvt_q_s,
  rv_op_fcvt_d_q,
  rv_op_fcvt_q_d,
  rv_op_fsqrt_q,
  rv_op_fle_q,
  rv_op_flt_q,
  rv_op_feq_q,
  rv_op_fcvt_w_q,
  rv_op_fcvt_wu_q,
  rv_op_fcvt_q_w,
  rv_op_fcvt_q_wu,
  rv_op_fclass_q,
  rv_op_fcvt_l_q,
  rv_op_fcvt_lu_q,
  rv_op_fcvt_q_l,
  rv_op_fcvt_q_lu,
  rv_op_fmv_x_q,
  rv_op_fmv_q_x,
  rv_op_c_addi4spn,
  rv_op_c_fld,
  rv_op_c_lw,
  rv_op_c_flw,
  rv_op_c_fsd,
  rv_op_c_sw,
  rv_op_c_fsw,
  rv_op_c_nop,
  rv_op_c_addi,
  rv_op_c_jal,
  rv_op_c_li,
  rv_op_c_addi16sp,
  rv_op_c_lui,
  rv_op_c_srli,
  rv_op_c_srai,
  rv_op_c_andi,
  rv_op_c_sub,
  rv_op_c_xor,
  rv_op_c_or,
  rv_op_c_and,
  rv_op_c_subw,
  rv_op_c_addw,
  rv_op_c_j,
  rv_op_c_beqz,
  rv_op_c_bnez,
  rv_op_c_slli,
  rv_op_c_fldsp,
  rv_op_c_lwsp,
  rv_op_c_flwsp,
  rv_op_c_jr,
  rv_op_c_mv,
  rv_op_c_ebreak,
  rv_op_c_jalr,
  rv_op_c_add,
  rv_op_c_fsdsp,
  rv_op_c_swsp,
  rv_op_c_fswsp,
  rv_op_c_ld,
  rv_op_c_sd,
  rv_op_c_addiw,
  rv_op_c_ldsp,
  rv_op_c_sdsp,
  rv_op_c_lq,
  rv_op_c_sq,
  rv_op_c_lqsp,
  rv_op_c_sqsp,
  rv_op_nop,
  rv_op_mv,
  rv_op_not,
  rv_op_neg,
  rv_op_negw,
  rv_op_sext_w,
  rv_op_seqz,
  rv_op_snez,
  rv_op_sltz,
  rv_op_sgtz,
  rv_op_fmv_s,
  rv_op_fabs_s,
  rv_op_fneg_s,
  rv_op_fmv_d,
  rv_op_fabs_d,
  rv_op_fneg_d,
  rv_op_fmv_q,
  rv_op_fabs_q,
  rv_op_fneg_q,
  rv_op_beqz,
  rv_op_bnez,
  rv_op_blez,
  rv_op_bgez,
  rv_op_bltz,
  rv_op_bgtz,
  rv_op_ble,
  rv_op_bleu,
  rv_op_bgt,
  rv_op_bgtu,
  rv_op_j,
  rv_op_ret,
  rv_op_jr,
  rv_op_rdcycle,
  rv_op_rdtime,
  rv_op_rdinstret,
  rv_op_rdcycleh,
  rv_op_rdtimeh,
  rv_op_rdinstreth,
  rv_op_frcsr,
  rv_op_frrm,
  rv_op_frflags,
  rv_op_fscsr,
  rv_op_fsrm,
  rv_op_fsflags,
  rv_op_fsrmi,
  rv_op_fsflagsi,
} rv_op;

/* structures */

typedef struct {
  uint64_t pc;
  uint64_t inst;
  int32_t imm;
  uint16_t op;
  uint8_t encodingType;
  uint8_t rd;
  uint8_t rs1;
  uint8_t rs2;
  uint8_t rs3;
  uint8_t rm;
  uint8_t pred;
  uint8_t succ;
  uint8_t aq;
  uint8_t rl;
} rv_instInfo;

typedef struct {
  const int32_t op;
  const rvc_constraint *constraints;
} rv_compData;

enum { rvcd_imm_nz = 0x1, rvcd_imm_nz_hint = 0x2 };

typedef struct {
  const char *const name;
  const rv_encodingType encodingType;
  const char *const format;
  const rv_compData *pseudo;
  const short decomp_rv32;
  const short decomp_rv64;
  const short decomp_rv128;
  const short decomp_data;
} rv_opcodeData;

/* register names */

static const char rv_ireg_name_sym[32][5] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

static const char rv_freg_name_sym[32][5] = {
    "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
    "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
    "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
};

/* instruction formats */

static const char rv_fmt_none[] = "O\t";
static const char rv_fmt_rs1[] = "O\t1";
static const char rv_fmt_offset[] = "O\to";
static const char rv_fmt_pred_succ[] = "O\tp,s";
static const char rv_fmt_rs1_rs2[] = "O\t1,2";
static const char rv_fmt_rd_imm[] = "O\t0,i";
static const char rv_fmt_rd_offset[] = "O\t0,o";
static const char rv_fmt_rd_rs1_rs2[] = "O\t0,1,2";
static const char rv_fmt_frd_rs1[] = "O\t3,1";
static const char rv_fmt_rd_frs1[] = "O\t0,4";
static const char rv_fmt_rd_frs1_frs2[] = "O\t0,4,5";
static const char rv_fmt_frd_frs1_frs2[] = "O\t3,4,5";
static const char rv_fmt_rm_frd_frs1[] = "O\tr,3,4";
static const char rv_fmt_rm_frd_rs1[] = "O\tr,3,1";
static const char rv_fmt_rm_rd_frs1[] = "O\tr,0,4";
static const char rv_fmt_rm_frd_frs1_frs2[] = "O\tr,3,4,5";
static const char rv_fmt_rm_frd_frs1_frs2_frs3[] = "O\tr,3,4,5,6";
static const char rv_fmt_rd_rs1_imm[] = "O\t0,1,i";
static const char rv_fmt_rd_rs1_offset[] = "O\t0,1,i";
static const char rv_fmt_rd_offset_rs1[] = "O\t0,i(1)";
static const char rv_fmt_frd_offset_rs1[] = "O\t3,i(1)";
static const char rv_fmt_rd_csr_rs1[] = "O\t0,c,1";
static const char rv_fmt_rd_csr_zimm[] = "O\t0,c,7";
static const char rv_fmt_rs2_offset_rs1[] = "O\t2,i(1)";
static const char rv_fmt_frs2_offset_rs1[] = "O\t5,i(1)";
static const char rv_fmt_rs1_rs2_offset[] = "O\t1,2,o";
static const char rv_fmt_rs2_rs1_offset[] = "O\t2,1,o";
static const char rv_fmt_aqrl_rd_rs2_rs1[] = "OAR\t0,2,(1)";
static const char rv_fmt_aqrl_rd_rs1[] = "OAR\t0,(1)";
static const char rv_fmt_rd[] = "O\t0";
static const char rv_fmt_rd_zimm[] = "O\t0,7";
static const char rv_fmt_rd_rs1[] = "O\t0,1";
static const char rv_fmt_rd_rs2[] = "O\t0,2";
static const char rv_fmt_rs1_offset[] = "O\t1,o";
static const char rv_fmt_rs2_offset[] = "O\t2,o";

/* pseudo-instruction constraints */

static const rvc_constraint rvcc_last[] = {rvc_end};
static const rvc_constraint rvcc_imm_eq_zero[] = {rvc_imm_eq_zero, rvc_end};
static const rvc_constraint rvcc_imm_eq_n1[] = {rvc_imm_eq_n1, rvc_end};
static const rvc_constraint rvcc_imm_eq_p1[] = {rvc_imm_eq_p1, rvc_end};
static const rvc_constraint rvcc_rs1_eq_x0[] = {rvc_rs1_eq_x0, rvc_end};
static const rvc_constraint rvcc_rs2_eq_x0[] = {rvc_rs2_eq_x0, rvc_end};
static const rvc_constraint rvcc_rs2_eq_rs1[] = {rvc_rs2_eq_rs1, rvc_end};
static const rvc_constraint rvcc_jal_j[] = {rvc_rd_eq_x0, rvc_end};
static const rvc_constraint rvcc_jal_jal[] = {rvc_rd_eq_ra, rvc_end};
static const rvc_constraint rvcc_jalr_jr[] = {rvc_rd_eq_x0, rvc_imm_eq_zero,
                                              rvc_end};
static const rvc_constraint rvcc_jalr_jalr[] = {rvc_rd_eq_ra, rvc_imm_eq_zero,
                                                rvc_end};
static const rvc_constraint rvcc_jalr_ret[] = {rvc_rd_eq_x0, rvc_rs1_eq_ra,
                                               rvc_end};
static const rvc_constraint rvcc_addi_nop[] = {rvc_rd_eq_x0, rvc_rs1_eq_x0,
                                               rvc_imm_eq_zero, rvc_end};
static const rvc_constraint rvcc_rdcycle[] = {rvc_rs1_eq_x0, rvc_csr_eq_0xc00,
                                              rvc_end};
static const rvc_constraint rvcc_rdtime[] = {rvc_rs1_eq_x0, rvc_csr_eq_0xc01,
                                             rvc_end};
static const rvc_constraint rvcc_rdinstret[] = {rvc_rs1_eq_x0, rvc_csr_eq_0xc02,
                                                rvc_end};
static const rvc_constraint rvcc_rdcycleh[] = {rvc_rs1_eq_x0, rvc_csr_eq_0xc80,
                                               rvc_end};
static const rvc_constraint rvcc_rdtimeh[] = {rvc_rs1_eq_x0, rvc_csr_eq_0xc81,
                                              rvc_end};
static const rvc_constraint rvcc_rdinstreth[] = {rvc_rs1_eq_x0,
                                                 rvc_csr_eq_0xc82, rvc_end};
static const rvc_constraint rvcc_frcsr[] = {rvc_rs1_eq_x0, rvc_csr_eq_0x003,
                                            rvc_end};
static const rvc_constraint rvcc_frrm[] = {rvc_rs1_eq_x0, rvc_csr_eq_0x002,
                                           rvc_end};
static const rvc_constraint rvcc_frflags[] = {rvc_rs1_eq_x0, rvc_csr_eq_0x001,
                                              rvc_end};
static const rvc_constraint rvcc_fscsr[] = {rvc_csr_eq_0x003, rvc_end};
static const rvc_constraint rvcc_fsrm[] = {rvc_csr_eq_0x002, rvc_end};
static const rvc_constraint rvcc_fsflags[] = {rvc_csr_eq_0x001, rvc_end};
static const rvc_constraint rvcc_fsrmi[] = {rvc_csr_eq_0x002, rvc_end};
static const rvc_constraint rvcc_fsflagsi[] = {rvc_csr_eq_0x001, rvc_end};

/* pseudo-instruction metadata */

static const rv_compData rvcp_jal[] = {
    {rv_op_j, rvcc_jal_j}, {rv_op_jal, rvcc_jal_jal}, {rv_op_illegal, NULL}};

static const rv_compData rvcp_jalr[] = {{rv_op_ret, rvcc_jalr_ret},
                                        {rv_op_jr, rvcc_jalr_jr},
                                        {rv_op_jalr, rvcc_jalr_jalr},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_beq[] = {{rv_op_beqz, rvcc_rs2_eq_x0},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_bne[] = {{rv_op_bnez, rvcc_rs2_eq_x0},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_blt[] = {{rv_op_bltz, rvcc_rs2_eq_x0},
                                       {rv_op_bgtz, rvcc_rs1_eq_x0},
                                       {rv_op_bgt, rvcc_last},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_bge[] = {{rv_op_blez, rvcc_rs1_eq_x0},
                                       {rv_op_bgez, rvcc_rs2_eq_x0},
                                       {rv_op_ble, rvcc_last},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_bltu[] = {{rv_op_bgtu, rvcc_last},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_bgeu[] = {{rv_op_bleu, rvcc_last},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_addi[] = {{rv_op_nop, rvcc_addi_nop},
                                        {rv_op_mv, rvcc_imm_eq_zero},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_sltiu[] = {{rv_op_seqz, rvcc_imm_eq_p1},
                                         {rv_op_illegal, NULL}};

static const rv_compData rvcp_xori[] = {{rv_op_not, rvcc_imm_eq_n1},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_sub[] = {{rv_op_neg, rvcc_rs1_eq_x0},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_slt[] = {{rv_op_sltz, rvcc_rs2_eq_x0},
                                       {rv_op_sgtz, rvcc_rs1_eq_x0},
                                       {rv_op_illegal, NULL}};

static const rv_compData rvcp_sltu[] = {{rv_op_snez, rvcc_rs1_eq_x0},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_addiw[] = {{rv_op_sext_w, rvcc_imm_eq_zero},
                                         {rv_op_illegal, NULL}};

static const rv_compData rvcp_subw[] = {{rv_op_negw, rvcc_rs1_eq_x0},
                                        {rv_op_illegal, NULL}};

static const rv_compData rvcp_csrrw[] = {{rv_op_fscsr, rvcc_fscsr},
                                         {rv_op_fsrm, rvcc_fsrm},
                                         {rv_op_fsflags, rvcc_fsflags},
                                         {rv_op_illegal, NULL}};

static const rv_compData rvcp_csrrs[] = {
    {rv_op_rdcycle, rvcc_rdcycle},     {rv_op_rdtime, rvcc_rdtime},
    {rv_op_rdinstret, rvcc_rdinstret}, {rv_op_rdcycleh, rvcc_rdcycleh},
    {rv_op_rdtimeh, rvcc_rdtimeh},     {rv_op_rdinstreth, rvcc_rdinstreth},
    {rv_op_frcsr, rvcc_frcsr},         {rv_op_frrm, rvcc_frrm},
    {rv_op_frflags, rvcc_frflags},     {rv_op_illegal, NULL}};

static const rv_compData rvcp_csrrwi[] = {{rv_op_fsrmi, rvcc_fsrmi},
                                          {rv_op_fsflagsi, rvcc_fsflagsi},
                                          {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnj_s[] = {{rv_op_fmv_s, rvcc_rs2_eq_rs1},
                                           {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjn_s[] = {{rv_op_fneg_s, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjx_s[] = {{rv_op_fabs_s, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnj_d[] = {{rv_op_fmv_d, rvcc_rs2_eq_rs1},
                                           {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjn_d[] = {{rv_op_fneg_d, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjx_d[] = {{rv_op_fabs_d, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnj_q[] = {{rv_op_fmv_q, rvcc_rs2_eq_rs1},
                                           {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjn_q[] = {{rv_op_fneg_q, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

static const rv_compData rvcp_fsgnjx_q[] = {{rv_op_fabs_q, rvcc_rs2_eq_rs1},
                                            {rv_op_illegal, NULL}};

/* instruction metadata */

const rv_opcodeData opcode_data[] = {
    {"illegal", rv_encodingType_illegal, rv_fmt_none, NULL, 0, 0, 0},
    {"lui", rv_encodingType_u, rv_fmt_rd_imm, NULL, 0, 0, 0},
    {"auipc", rv_encodingType_u, rv_fmt_rd_offset, NULL, 0, 0, 0},
    {"jal", rv_encodingType_uj, rv_fmt_rd_offset, rvcp_jal, 0, 0, 0},
    {"jalr", rv_encodingType_i, rv_fmt_rd_rs1_offset, rvcp_jalr, 0, 0, 0},
    {"beq", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_beq, 0, 0, 0},
    {"bne", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_bne, 0, 0, 0},
    {"blt", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_blt, 0, 0, 0},
    {"bge", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_bge, 0, 0, 0},
    {"bltu", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_bltu, 0, 0, 0},
    {"bgeu", rv_encodingType_sb, rv_fmt_rs1_rs2_offset, rvcp_bgeu, 0, 0, 0},
    {"lb", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"lh", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"lw", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"lbu", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"lhu", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"sb", rv_encodingType_s, rv_fmt_rs2_offset_rs1, NULL, 0, 0, 0},
    {"sh", rv_encodingType_s, rv_fmt_rs2_offset_rs1, NULL, 0, 0, 0},
    {"sw", rv_encodingType_s, rv_fmt_rs2_offset_rs1, NULL, 0, 0, 0},
    {"addi", rv_encodingType_i, rv_fmt_rd_rs1_imm, rvcp_addi, 0, 0, 0},
    {"slti", rv_encodingType_i, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"sltiu", rv_encodingType_i, rv_fmt_rd_rs1_imm, rvcp_sltiu, 0, 0, 0},
    {"xori", rv_encodingType_i, rv_fmt_rd_rs1_imm, rvcp_xori, 0, 0, 0},
    {"ori", rv_encodingType_i, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"andi", rv_encodingType_i, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"slli", rv_encodingType_i_sh7, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"srli", rv_encodingType_i_sh7, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"srai", rv_encodingType_i_sh7, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"add", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"sub", rv_encodingType_r, rv_fmt_rd_rs1_rs2, rvcp_sub, 0, 0, 0},
    {"sll", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"slt", rv_encodingType_r, rv_fmt_rd_rs1_rs2, rvcp_slt, 0, 0, 0},
    {"sltu", rv_encodingType_r, rv_fmt_rd_rs1_rs2, rvcp_sltu, 0, 0, 0},
    {"xor", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"srl", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"sra", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"or", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"and", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"fence", rv_encodingType_r_f, rv_fmt_pred_succ, NULL, 0, 0, 0},
    {"fence.i", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"lwu", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"ld", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"sd", rv_encodingType_s, rv_fmt_rs2_offset_rs1, NULL, 0, 0, 0},
    {"addiw", rv_encodingType_i, rv_fmt_rd_rs1_imm, rvcp_addiw, 0, 0, 0},
    {"slliw", rv_encodingType_i_sh5, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"srliw", rv_encodingType_i_sh5, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"sraiw", rv_encodingType_i_sh5, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"addw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"subw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, rvcp_subw, 0, 0, 0},
    {"sllw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"srlw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"sraw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"ldu", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"lq", rv_encodingType_i, rv_fmt_rd_offset_rs1, NULL, 0, 0, 0},
    {"sq", rv_encodingType_s, rv_fmt_rs2_offset_rs1, NULL, 0, 0, 0},
    {"addid", rv_encodingType_i, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"sllid", rv_encodingType_i_sh6, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"srlid", rv_encodingType_i_sh6, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"sraid", rv_encodingType_i_sh6, rv_fmt_rd_rs1_imm, NULL, 0, 0, 0},
    {"addd", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"subd", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"slld", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"srld", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"srad", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"mul", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"mulh", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"mulhsu", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"mulhu", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"div", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"divu", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"rem", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"remu", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"mulw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"divw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"divuw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"remw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"remuw", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"muld", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"divd", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"divud", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"remd", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"remud", rv_encodingType_r, rv_fmt_rd_rs1_rs2, NULL, 0, 0, 0},
    {"lr.w", rv_encodingType_r_l, rv_fmt_aqrl_rd_rs1, NULL, 0, 0, 0},
    {"sc.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoswap.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoadd.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoxor.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoor.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoand.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomin.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomax.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amominu.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomaxu.w", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"lr.d", rv_encodingType_r_l, rv_fmt_aqrl_rd_rs1, NULL, 0, 0, 0},
    {"sc.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoswap.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoadd.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoxor.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoor.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoand.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomin.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomax.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amominu.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomaxu.d", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"lr.q", rv_encodingType_r_l, rv_fmt_aqrl_rd_rs1, NULL, 0, 0, 0},
    {"sc.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoswap.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoadd.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoxor.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoor.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amoand.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomin.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomax.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amominu.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"amomaxu.q", rv_encodingType_r_a, rv_fmt_aqrl_rd_rs2_rs1, NULL, 0, 0, 0},
    {"ecall", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"ebreak", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"uret", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"sret", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"hret", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"mret", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"dret", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"sfence.vm", rv_encodingType_r, rv_fmt_rs1, NULL, 0, 0, 0},
    {"sfence.vma", rv_encodingType_r, rv_fmt_rs1_rs2, NULL, 0, 0, 0},
    {"wfi", rv_encodingType_none, rv_fmt_none, NULL, 0, 0, 0},
    {"csrrw", rv_encodingType_i_csr, rv_fmt_rd_csr_rs1, rvcp_csrrw, 0, 0, 0},
    {"csrrs", rv_encodingType_i_csr, rv_fmt_rd_csr_rs1, rvcp_csrrs, 0, 0, 0},
    {"csrrc", rv_encodingType_i_csr, rv_fmt_rd_csr_rs1, NULL, 0, 0, 0},
    {"csrrwi", rv_encodingType_i_csr, rv_fmt_rd_csr_zimm, rvcp_csrrwi, 0, 0, 0},
    {"csrrsi", rv_encodingType_i_csr, rv_fmt_rd_csr_zimm, NULL, 0, 0, 0},
    {"csrrci", rv_encodingType_i_csr, rv_fmt_rd_csr_zimm, NULL, 0, 0, 0},
    {"flw", rv_encodingType_i, rv_fmt_frd_offset_rs1, NULL, 0, 0, 0},
    {"fsw", rv_encodingType_s, rv_fmt_frs2_offset_rs1, NULL, 0, 0, 0},
    {"fmadd.s", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fmsub.s", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmsub.s", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmadd.s", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fadd.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsub.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmul.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fdiv.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsgnj.s", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnj_s, 0, 0, 0},
    {"fsgnjn.s", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjn_s, 0, 0,
     0},
    {"fsgnjx.s", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjx_s, 0, 0,
     0},
    {"fmin.s", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmax.s", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsqrt.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fle.s", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"flt.s", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"feq.s", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"fcvt.w.s", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.wu.s", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.s.w", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.s.wu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fmv.x.s", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fclass.s", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fmv.s.x", rv_encodingType_r, rv_fmt_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.l.s", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.lu.s", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.s.l", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.s.lu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fld", rv_encodingType_i, rv_fmt_frd_offset_rs1, NULL, 0, 0, 0},
    {"fsd", rv_encodingType_s, rv_fmt_frs2_offset_rs1, NULL, 0, 0, 0},
    {"fmadd.d", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fmsub.d", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmsub.d", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmadd.d", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fadd.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsub.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmul.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fdiv.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsgnj.d", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnj_d, 0, 0, 0},
    {"fsgnjn.d", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjn_d, 0, 0,
     0},
    {"fsgnjx.d", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjx_d, 0, 0,
     0},
    {"fmin.d", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmax.d", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fcvt.s.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fcvt.d.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fsqrt.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fle.d", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"flt.d", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"feq.d", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"fcvt.w.d", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.wu.d", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.d.w", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.d.wu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fclass.d", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.l.d", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.lu.d", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fmv.x.d", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.d.l", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.d.lu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fmv.d.x", rv_encodingType_r, rv_fmt_frd_rs1, NULL, 0, 0, 0},
    {"flq", rv_encodingType_i, rv_fmt_frd_offset_rs1, NULL, 0, 0, 0},
    {"fsq", rv_encodingType_s, rv_fmt_frs2_offset_rs1, NULL, 0, 0, 0},
    {"fmadd.q", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fmsub.q", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmsub.q", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fnmadd.q", rv_encodingType_r4_m, rv_fmt_rm_frd_frs1_frs2_frs3, NULL, 0, 0,
     0},
    {"fadd.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsub.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmul.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fdiv.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fsgnj.q", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnj_q, 0, 0, 0},
    {"fsgnjn.q", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjn_q, 0, 0,
     0},
    {"fsgnjx.q", rv_encodingType_r, rv_fmt_frd_frs1_frs2, rvcp_fsgnjx_q, 0, 0,
     0},
    {"fmin.q", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fmax.q", rv_encodingType_r, rv_fmt_frd_frs1_frs2, NULL, 0, 0, 0},
    {"fcvt.s.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fcvt.q.s", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fcvt.d.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fcvt.q.d", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fsqrt.q", rv_encodingType_r_m, rv_fmt_rm_frd_frs1, NULL, 0, 0, 0},
    {"fle.q", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"flt.q", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"feq.q", rv_encodingType_r, rv_fmt_rd_frs1_frs2, NULL, 0, 0, 0},
    {"fcvt.w.q", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.wu.q", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.q.w", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.q.wu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fclass.q", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.l.q", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.lu.q", rv_encodingType_r_m, rv_fmt_rm_rd_frs1, NULL, 0, 0, 0},
    {"fcvt.q.l", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fcvt.q.lu", rv_encodingType_r_m, rv_fmt_rm_frd_rs1, NULL, 0, 0, 0},
    {"fmv.x.q", rv_encodingType_r, rv_fmt_rd_frs1, NULL, 0, 0, 0},
    {"fmv.q.x", rv_encodingType_r, rv_fmt_frd_rs1, NULL, 0, 0, 0},
    {"c.addi4spn", rv_encodingType_ciw_4spn, rv_fmt_rd_rs1_imm, NULL,
     rv_op_addi, rv_op_addi, rv_op_addi, rvcd_imm_nz},
    {"c.fld", rv_encodingType_cl_ld, rv_fmt_frd_offset_rs1, NULL, rv_op_fld,
     rv_op_fld, 0},
    {"c.lw", rv_encodingType_cl_lw, rv_fmt_rd_offset_rs1, NULL, rv_op_lw,
     rv_op_lw, rv_op_lw},
    {"c.flw", rv_encodingType_cl_lw, rv_fmt_frd_offset_rs1, NULL, rv_op_flw, 0,
     0},
    {"c.fsd", rv_encodingType_cs_sd, rv_fmt_frs2_offset_rs1, NULL, rv_op_fsd,
     rv_op_fsd, 0},
    {"c.sw", rv_encodingType_cs_sw, rv_fmt_rs2_offset_rs1, NULL, rv_op_sw,
     rv_op_sw, rv_op_sw},
    {"c.fsw", rv_encodingType_cs_sw, rv_fmt_frs2_offset_rs1, NULL, rv_op_fsw, 0,
     0},
    {"c.nop", rv_encodingType_ci_none, rv_fmt_none, NULL, rv_op_addi,
     rv_op_addi, rv_op_addi},
    {"c.addi", rv_encodingType_ci, rv_fmt_rd_rs1_imm, NULL, rv_op_addi,
     rv_op_addi, rv_op_addi, rvcd_imm_nz_hint},
    {"c.jal", rv_encodingType_cj_jal, rv_fmt_rd_offset, NULL, rv_op_jal, 0, 0},
    {"c.li", rv_encodingType_ci_li, rv_fmt_rd_rs1_imm, NULL, rv_op_addi,
     rv_op_addi, rv_op_addi},
    {"c.addi16sp", rv_encodingType_ci_16sp, rv_fmt_rd_rs1_imm, NULL, rv_op_addi,
     rv_op_addi, rv_op_addi, rvcd_imm_nz},
    {"c.lui", rv_encodingType_ci_lui, rv_fmt_rd_imm, NULL, rv_op_lui, rv_op_lui,
     rv_op_lui, rvcd_imm_nz},
    {"c.srli", rv_encodingType_cb_sh6, rv_fmt_rd_rs1_imm, NULL, rv_op_srli,
     rv_op_srli, rv_op_srli, rvcd_imm_nz},
    {"c.srai", rv_encodingType_cb_sh6, rv_fmt_rd_rs1_imm, NULL, rv_op_srai,
     rv_op_srai, rv_op_srai, rvcd_imm_nz},
    {"c.andi", rv_encodingType_cb_imm, rv_fmt_rd_rs1_imm, NULL, rv_op_andi,
     rv_op_andi, rv_op_andi, rvcd_imm_nz},
    {"c.sub", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_sub, rv_op_sub,
     rv_op_sub},
    {"c.xor", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_xor, rv_op_xor,
     rv_op_xor},
    {"c.or", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_or, rv_op_or,
     rv_op_or},
    {"c.and", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_and, rv_op_and,
     rv_op_and},
    {"c.subw", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_subw,
     rv_op_subw, rv_op_subw},
    {"c.addw", rv_encodingType_cs, rv_fmt_rd_rs1_rs2, NULL, rv_op_addw,
     rv_op_addw, rv_op_addw},
    {"c.j", rv_encodingType_cj, rv_fmt_rd_offset, NULL, rv_op_jal, rv_op_jal,
     rv_op_jal},
    {"c.beqz", rv_encodingType_cb, rv_fmt_rs1_rs2_offset, NULL, rv_op_beq,
     rv_op_beq, rv_op_beq},
    {"c.bnez", rv_encodingType_cb, rv_fmt_rs1_rs2_offset, NULL, rv_op_bne,
     rv_op_bne, rv_op_bne},
    {"c.slli", rv_encodingType_ci_sh6, rv_fmt_rd_rs1_imm, NULL, rv_op_slli,
     rv_op_slli, rv_op_slli, rvcd_imm_nz},
    {"c.fldsp", rv_encodingType_ci_ldsp, rv_fmt_frd_offset_rs1, NULL, rv_op_fld,
     rv_op_fld, rv_op_fld},
    {"c.lwsp", rv_encodingType_ci_lwsp, rv_fmt_rd_offset_rs1, NULL, rv_op_lw,
     rv_op_lw, rv_op_lw},
    {"c.flwsp", rv_encodingType_ci_lwsp, rv_fmt_frd_offset_rs1, NULL, rv_op_flw,
     0, 0},
    {"c.jr", rv_encodingType_cr_jr, rv_fmt_rd_rs1_offset, NULL, rv_op_jalr,
     rv_op_jalr, rv_op_jalr},
    {"c.mv", rv_encodingType_cr_mv, rv_fmt_rd_rs1_rs2, NULL, rv_op_addi,
     rv_op_addi, rv_op_addi},
    {"c.ebreak", rv_encodingType_ci_none, rv_fmt_none, NULL, rv_op_ebreak,
     rv_op_ebreak, rv_op_ebreak},
    {"c.jalr", rv_encodingType_cr_jalr, rv_fmt_rd_rs1_offset, NULL, rv_op_jalr,
     rv_op_jalr, rv_op_jalr},
    {"c.add", rv_encodingType_cr, rv_fmt_rd_rs1_rs2, NULL, rv_op_add, rv_op_add,
     rv_op_add},
    {"c.fsdsp", rv_encodingType_css_sdsp, rv_fmt_frs2_offset_rs1, NULL,
     rv_op_fsd, rv_op_fsd, rv_op_fsd},
    {"c.swsp", rv_encodingType_css_swsp, rv_fmt_rs2_offset_rs1, NULL, rv_op_sw,
     rv_op_sw, rv_op_sw},
    {"c.fswsp", rv_encodingType_css_swsp, rv_fmt_frs2_offset_rs1, NULL,
     rv_op_fsw, 0, 0},
    {"c.ld", rv_encodingType_cl_ld, rv_fmt_rd_offset_rs1, NULL, 0, rv_op_ld,
     rv_op_ld},
    {"c.sd", rv_encodingType_cs_sd, rv_fmt_rs2_offset_rs1, NULL, 0, rv_op_sd,
     rv_op_sd},
    {"c.addiw", rv_encodingType_ci, rv_fmt_rd_rs1_imm, NULL, 0, rv_op_addiw,
     rv_op_addiw},
    {"c.ldsp", rv_encodingType_ci_ldsp, rv_fmt_rd_offset_rs1, NULL, 0, rv_op_ld,
     rv_op_ld},
    {"c.sdsp", rv_encodingType_css_sdsp, rv_fmt_rs2_offset_rs1, NULL, 0,
     rv_op_sd, rv_op_sd},
    {"c.lq", rv_encodingType_cl_lq, rv_fmt_rd_offset_rs1, NULL, 0, 0, rv_op_lq},
    {"c.sq", rv_encodingType_cs_sq, rv_fmt_rs2_offset_rs1, NULL, 0, 0,
     rv_op_sq},
    {"c.lqsp", rv_encodingType_ci_lqsp, rv_fmt_rd_offset_rs1, NULL, 0, 0,
     rv_op_lq},
    {"c.sqsp", rv_encodingType_css_sqsp, rv_fmt_rs2_offset_rs1, NULL, 0, 0,
     rv_op_sq},
    {"nop", rv_encodingType_i, rv_fmt_none, NULL, 0, 0, 0},
    {"mv", rv_encodingType_i, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"not", rv_encodingType_i, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"neg", rv_encodingType_r, rv_fmt_rd_rs2, NULL, 0, 0, 0},
    {"negw", rv_encodingType_r, rv_fmt_rd_rs2, NULL, 0, 0, 0},
    {"sext.w", rv_encodingType_i, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"seqz", rv_encodingType_i, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"snez", rv_encodingType_r, rv_fmt_rd_rs2, NULL, 0, 0, 0},
    {"sltz", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"sgtz", rv_encodingType_r, rv_fmt_rd_rs2, NULL, 0, 0, 0},
    {"fmv.s", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fabs.s", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fneg.s", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fmv.d", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fabs.d", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fneg.d", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fmv.q", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fabs.q", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fneg.q", rv_encodingType_r, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"beqz", rv_encodingType_sb, rv_fmt_rs1_offset, NULL, 0, 0, 0},
    {"bnez", rv_encodingType_sb, rv_fmt_rs1_offset, NULL, 0, 0, 0},
    {"blez", rv_encodingType_sb, rv_fmt_rs2_offset, NULL, 0, 0, 0},
    {"bgez", rv_encodingType_sb, rv_fmt_rs1_offset, NULL, 0, 0, 0},
    {"bltz", rv_encodingType_sb, rv_fmt_rs1_offset, NULL, 0, 0, 0},
    {"bgtz", rv_encodingType_sb, rv_fmt_rs2_offset, NULL, 0, 0, 0},
    {"ble", rv_encodingType_sb, rv_fmt_rs2_rs1_offset, NULL, 0, 0, 0},
    {"bleu", rv_encodingType_sb, rv_fmt_rs2_rs1_offset, NULL, 0, 0, 0},
    {"bgt", rv_encodingType_sb, rv_fmt_rs2_rs1_offset, NULL, 0, 0, 0},
    {"bgtu", rv_encodingType_sb, rv_fmt_rs2_rs1_offset, NULL, 0, 0, 0},
    {"j", rv_encodingType_uj, rv_fmt_offset, NULL, 0, 0, 0},
    {"ret", rv_encodingType_i, rv_fmt_none, NULL, 0, 0, 0},
    {"jr", rv_encodingType_i, rv_fmt_rs1, NULL, 0, 0, 0},
    {"rdcycle", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"rdtime", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"rdinstret", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"rdcycleh", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"rdtimeh", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"rdinstreth", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"frcsr", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"frrm", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"frflags", rv_encodingType_i_csr, rv_fmt_rd, NULL, 0, 0, 0},
    {"fscsr", rv_encodingType_i_csr, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fsrm", rv_encodingType_i_csr, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fsflags", rv_encodingType_i_csr, rv_fmt_rd_rs1, NULL, 0, 0, 0},
    {"fsrmi", rv_encodingType_i_csr, rv_fmt_rd_zimm, NULL, 0, 0, 0},
    {"fsflagsi", rv_encodingType_i_csr, rv_fmt_rd_zimm, NULL, 0, 0, 0},
};

/* instruction length */

static size_t inst_length(rv_inst inst) {
  /* NOTE: supports maximum instruction size of 64-bits */

  /* instruction length coding standard
   *
   *      aa - 16 bit aa != 11
   *   bbb11 - 32 bit bbb != 111
   *  011111 - 48 bit
   * 0111111 - 64 bit
   */

  return (inst & 0b11) != 0b11             ? 2
         : (inst & 0b11100) != 0b11100     ? 4
         : (inst & 0b111111) == 0b011111   ? 6
         : (inst & 0b1111111) == 0b0111111 ? 8
                                           : 0;
}

/* CSR names */

static const char *csr_name(int csrno) {
  switch (csrno) {
  case 0x0000:
    return "ustatus";
  case 0x0001:
    return "fflags";
  case 0x0002:
    return "frm";
  case 0x0003:
    return "fcsr";
  case 0x0004:
    return "uie";
  case 0x0005:
    return "utvec";
  case 0x0007:
    return "utvt";
  case 0x0008:
    return "vstart";
  case 0x0009:
    return "vxsat";
  case 0x000a:
    return "vxrm";
  case 0x000f:
    return "vcsr";
  case 0x0040:
    return "uscratch";
  case 0x0041:
    return "uepc";
  case 0x0042:
    return "ucause";
  case 0x0043:
    return "utval";
  case 0x0044:
    return "uip";
  case 0x0045:
    return "unxti";
  case 0x0046:
    return "uintstatus";
  case 0x0048:
    return "uscratchcsw";
  case 0x0049:
    return "uscratchcswl";
  case 0x0100:
    return "sstatus";
  case 0x0102:
    return "sedeleg";
  case 0x0103:
    return "sideleg";
  case 0x0104:
    return "sie";
  case 0x0105:
    return "stvec";
  case 0x0106:
    return "scounteren";
  case 0x0107:
    return "stvt";
  case 0x0140:
    return "sscratch";
  case 0x0141:
    return "sepc";
  case 0x0142:
    return "scause";
  case 0x0143:
    return "stval";
  case 0x0144:
    return "sip";
  case 0x0145:
    return "snxti";
  case 0x0146:
    return "sintstatus";
  case 0x0148:
    return "sscratchcsw";
  case 0x0149:
    return "sscratchcswl";
  case 0x0180:
    return "satp";
  case 0x0200:
    return "vsstatus";
  case 0x0204:
    return "vsie";
  case 0x0205:
    return "vstvec";
  case 0x0240:
    return "vsscratch";
  case 0x0241:
    return "vsepc";
  case 0x0242:
    return "vscause";
  case 0x0243:
    return "vstval";
  case 0x0244:
    return "vsip";
  case 0x0280:
    return "vsatp";
  case 0x0300:
    return "mstatus";
  case 0x0301:
    return "misa";
  case 0x0302:
    return "medeleg";
  case 0x0303:
    return "mideleg";
  case 0x0304:
    return "mie";
  case 0x0305:
    return "mtvec";
  case 0x0306:
    return "mcounteren";
  case 0x0307:
    return "mtvt";
  case 0x0310:
    return "mstatush";
  case 0x0320:
    return "mcountinhibit";
  case 0x0323:
    return "mhpmevent3";
  case 0x0324:
    return "mhpmevent4";
  case 0x0325:
    return "mhpmevent5";
  case 0x0326:
    return "mhpmevent6";
  case 0x0327:
    return "mhpmevent7";
  case 0x0328:
    return "mhpmevent8";
  case 0x0329:
    return "mhpmevent9";
  case 0x032a:
    return "mhpmevent10";
  case 0x032b:
    return "mhpmevent11";
  case 0x032c:
    return "mhpmevent12";
  case 0x032d:
    return "mhpmevent13";
  case 0x032e:
    return "mhpmevent14";
  case 0x032f:
    return "mhpmevent15";
  case 0x0330:
    return "mhpmevent16";
  case 0x0331:
    return "mhpmevent17";
  case 0x0332:
    return "mhpmevent18";
  case 0x0333:
    return "mhpmevent19";
  case 0x0334:
    return "mhpmevent20";
  case 0x0335:
    return "mhpmevent21";
  case 0x0336:
    return "mhpmevent22";
  case 0x0337:
    return "mhpmevent23";
  case 0x0338:
    return "mhpmevent24";
  case 0x0339:
    return "mhpmevent25";
  case 0x033a:
    return "mhpmevent26";
  case 0x033b:
    return "mhpmevent27";
  case 0x033c:
    return "mhpmevent28";
  case 0x033d:
    return "mhpmevent29";
  case 0x033e:
    return "mhpmevent30";
  case 0x033f:
    return "mhpmevent31";
  case 0x0340:
    return "mscratch";
  case 0x0341:
    return "mepc";
  case 0x0342:
    return "mcause";
  case 0x0343:
    return "mtval";
  case 0x0344:
    return "mip";
  case 0x0345:
    return "mnxti";
  case 0x0346:
    return "mintstatus";
  case 0x0348:
    return "mscratchcsw";
  case 0x0349:
    return "mscratchcswl";
  case 0x034a:
    return "mtinst";
  case 0x034b:
    return "mtval2";
  case 0x03a0:
    return "pmpcfg0";
  case 0x03a1:
    return "pmpcfg1";
  case 0x03a2:
    return "pmpcfg2";
  case 0x03a3:
    return "pmpcfg3";
  case 0x03b0:
    return "pmpaddr0";
  case 0x03b1:
    return "pmpaddr1";
  case 0x03b2:
    return "pmpaddr2";
  case 0x03b3:
    return "pmpaddr3";
  case 0x03b4:
    return "pmpaddr4";
  case 0x03b5:
    return "pmpaddr5";
  case 0x03b6:
    return "pmpaddr6";
  case 0x03b7:
    return "pmpaddr7";
  case 0x03b8:
    return "pmpaddr8";
  case 0x03b9:
    return "pmpaddr9";
  case 0x03ba:
    return "pmpaddr10";
  case 0x03bb:
    return "pmpaddr11";
  case 0x03bc:
    return "pmpaddr12";
  case 0x03bd:
    return "pmpaddr13";
  case 0x03be:
    return "pmpaddr14";
  case 0x03bf:
    return "pmpaddr15";
  case 0x0600:
    return "hstatus";
  case 0x0602:
    return "hedeleg";
  case 0x0603:
    return "hideleg";
  case 0x0604:
    return "hie";
  case 0x0605:
    return "htimedelta";
  case 0x0606:
    return "hcounteren";
  case 0x0607:
    return "hgeie";
  case 0x0615:
    return "htimedeltah";
  case 0x0643:
    return "htval";
  case 0x0644:
    return "hip";
  case 0x0645:
    return "hvip";
  case 0x064a:
    return "htinst";
  case 0x0680:
    return "hgatp";
  case 0x07a0:
    return "tselect";
  case 0x07a1:
    return "tdata1";
  case 0x07a2:
    return "tdata2";
  case 0x07a3:
    return "tdata3";
  case 0x07a4:
    return "tinfo";
  case 0x07a5:
    return "tcontrol";
  case 0x07a8:
    return "mcontext";
  case 0x07a9:
    return "mnoise";
  case 0x07aa:
    return "scontext";
  case 0x07b0:
    return "dcsr";
  case 0x07b1:
    return "dpc";
  case 0x07b2:
    return "dscratch0";
  case 0x07b3:
    return "dscratch1";
  case 0x0b00:
    return "mcycle";
  case 0x0b02:
    return "minstret";
  case 0x0b03:
    return "mhpmcounter3";
  case 0x0b04:
    return "mhpmcounter4";
  case 0x0b05:
    return "mhpmcounter5";
  case 0x0b06:
    return "mhpmcounter6";
  case 0x0b07:
    return "mhpmcounter7";
  case 0x0b08:
    return "mhpmcounter8";
  case 0x0b09:
    return "mhpmcounter9";
  case 0x0b0a:
    return "mhpmcounter10";
  case 0x0b0b:
    return "mhpmcounter11";
  case 0x0b0c:
    return "mhpmcounter12";
  case 0x0b0d:
    return "mhpmcounter13";
  case 0x0b0e:
    return "mhpmcounter14";
  case 0x0b0f:
    return "mhpmcounter15";
  case 0x0b10:
    return "mhpmcounter16";
  case 0x0b11:
    return "mhpmcounter17";
  case 0x0b12:
    return "mhpmcounter18";
  case 0x0b13:
    return "mhpmcounter19";
  case 0x0b14:
    return "mhpmcounter20";
  case 0x0b15:
    return "mhpmcounter21";
  case 0x0b16:
    return "mhpmcounter22";
  case 0x0b17:
    return "mhpmcounter23";
  case 0x0b18:
    return "mhpmcounter24";
  case 0x0b19:
    return "mhpmcounter25";
  case 0x0b1a:
    return "mhpmcounter26";
  case 0x0b1b:
    return "mhpmcounter27";
  case 0x0b1c:
    return "mhpmcounter28";
  case 0x0b1d:
    return "mhpmcounter29";
  case 0x0b1e:
    return "mhpmcounter30";
  case 0x0b1f:
    return "mhpmcounter31";
  case 0x0b80:
    return "mcycleh";
  case 0x0b82:
    return "minstreth";
  case 0x0b83:
    return "mhpmcounter3h";
  case 0x0b84:
    return "mhpmcounter4h";
  case 0x0b85:
    return "mhpmcounter5h";
  case 0x0b86:
    return "mhpmcounter6h";
  case 0x0b87:
    return "mhpmcounter7h";
  case 0x0b88:
    return "mhpmcounter8h";
  case 0x0b89:
    return "mhpmcounter9h";
  case 0x0b8a:
    return "mhpmcounter10h";
  case 0x0b8b:
    return "mhpmcounter11h";
  case 0x0b8c:
    return "mhpmcounter12h";
  case 0x0b8d:
    return "mhpmcounter13h";
  case 0x0b8e:
    return "mhpmcounter14h";
  case 0x0b8f:
    return "mhpmcounter15h";
  case 0x0b90:
    return "mhpmcounter16h";
  case 0x0b91:
    return "mhpmcounter17h";
  case 0x0b92:
    return "mhpmcounter18h";
  case 0x0b93:
    return "mhpmcounter19h";
  case 0x0b94:
    return "mhpmcounter20h";
  case 0x0b95:
    return "mhpmcounter21h";
  case 0x0b96:
    return "mhpmcounter22h";
  case 0x0b97:
    return "mhpmcounter23h";
  case 0x0b98:
    return "mhpmcounter24h";
  case 0x0b99:
    return "mhpmcounter25h";
  case 0x0b9a:
    return "mhpmcounter26h";
  case 0x0b9b:
    return "mhpmcounter27h";
  case 0x0b9c:
    return "mhpmcounter28h";
  case 0x0b9d:
    return "mhpmcounter29h";
  case 0x0b9e:
    return "mhpmcounter30h";
  case 0x0b9f:
    return "mhpmcounter31h";
  case 0x0c00:
    return "cycle";
  case 0x0c01:
    return "time";
  case 0x0c02:
    return "instret";
  case 0x0c03:
    return "hpmcounter3";
  case 0x0c04:
    return "hpmcounter4";
  case 0x0c05:
    return "hpmcounter5";
  case 0x0c06:
    return "hpmcounter6";
  case 0x0c07:
    return "hpmcounter7";
  case 0x0c08:
    return "hpmcounter8";
  case 0x0c09:
    return "hpmcounter9";
  case 0x0c0a:
    return "hpmcounter10";
  case 0x0c0b:
    return "hpmcounter11";
  case 0x0c0c:
    return "hpmcounter12";
  case 0x0c0d:
    return "hpmcounter13";
  case 0x0c0e:
    return "hpmcounter14";
  case 0x0c0f:
    return "hpmcounter15";
  case 0x0c10:
    return "hpmcounter16";
  case 0x0c11:
    return "hpmcounter17";
  case 0x0c12:
    return "hpmcounter18";
  case 0x0c13:
    return "hpmcounter19";
  case 0x0c14:
    return "hpmcounter20";
  case 0x0c15:
    return "hpmcounter21";
  case 0x0c16:
    return "hpmcounter22";
  case 0x0c17:
    return "hpmcounter23";
  case 0x0c18:
    return "hpmcounter24";
  case 0x0c19:
    return "hpmcounter25";
  case 0x0c1a:
    return "hpmcounter26";
  case 0x0c1b:
    return "hpmcounter27";
  case 0x0c1c:
    return "hpmcounter28";
  case 0x0c1d:
    return "hpmcounter29";
  case 0x0c1e:
    return "hpmcounter30";
  case 0x0c1f:
    return "hpmcounter31";
  case 0x0c20:
    return "vl";
  case 0x0c21:
    return "vtype";
  case 0x0c22:
    return "vlenb";
  case 0x0c80:
    return "cycleh";
  case 0x0c81:
    return "timeh";
  case 0x0c82:
    return "instreth";
  case 0x0c83:
    return "hpmcounter3h";
  case 0x0c84:
    return "hpmcounter4h";
  case 0x0c85:
    return "hpmcounter5h";
  case 0x0c86:
    return "hpmcounter6h";
  case 0x0c87:
    return "hpmcounter7h";
  case 0x0c88:
    return "hpmcounter8h";
  case 0x0c89:
    return "hpmcounter9h";
  case 0x0c8a:
    return "hpmcounter10h";
  case 0x0c8b:
    return "hpmcounter11h";
  case 0x0c8c:
    return "hpmcounter12h";
  case 0x0c8d:
    return "hpmcounter13h";
  case 0x0c8e:
    return "hpmcounter14h";
  case 0x0c8f:
    return "hpmcounter15h";
  case 0x0c90:
    return "hpmcounter16h";
  case 0x0c91:
    return "hpmcounter17h";
  case 0x0c92:
    return "hpmcounter18h";
  case 0x0c93:
    return "hpmcounter19h";
  case 0x0c94:
    return "hpmcounter20h";
  case 0x0c95:
    return "hpmcounter21h";
  case 0x0c96:
    return "hpmcounter22h";
  case 0x0c97:
    return "hpmcounter23h";
  case 0x0c98:
    return "hpmcounter24h";
  case 0x0c99:
    return "hpmcounter25h";
  case 0x0c9a:
    return "hpmcounter26h";
  case 0x0c9b:
    return "hpmcounter27h";
  case 0x0c9c:
    return "hpmcounter28h";
  case 0x0c9d:
    return "hpmcounter29h";
  case 0x0c9e:
    return "hpmcounter30h";
  case 0x0c9f:
    return "hpmcounter31h";
  case 0x0e12:
    return "hgeip";
  case 0x0f11:
    return "mvendorid";
  case 0x0f12:
    return "marchid";
  case 0x0f13:
    return "mimpid";
  case 0x0f14:
    return "mhartid";
  case 0x0f15:
    return "mentropy";
  default:
    return NULL;
  }
}

/* Parse opcode of all riscv instructions */

static void rv_inst_opcode(rv_instInfo *instInfo, rv_isa isa) {
  rv_inst inst = instInfo->inst;
  rv_opcode op = rv_op_illegal;
  switch (((inst >> 0) & 0b11)) {
  case 0: // compression instruction instLen=16bits
    switch (((inst >> 13) & 0b111)) {
    case 0:
      op = rv_op_c_addi4spn;
      break;
    case 1:
      op = (isa == rv128) ? rv_op_c_lq : rv_op_c_fld;
      break;
    case 2:
      op = rv_op_c_lw;
      break;
    case 3:
      op = (isa == rv32) ? rv_op_c_flw : rv_op_c_ld;
      break;
    case 5:
      op = (isa == rv128) ? rv_op_c_sq : rv_op_c_fsd;
      break;
    case 6:
      op = rv_op_c_sw;
      break;
    case 7:
      op = (isa == rv32) ? rv_op_c_fsw : rv_op_c_sd;
      break;
    }
    break;
  case 1: // compression instruction instLen=16bits
    switch (((inst >> 13) & 0b111)) {
    case 0:
      switch (((inst >> 2) & 0b11111111111)) {
      case 0:
        op = rv_op_c_nop;
        break;
      default:
        op = rv_op_c_addi;
        break;
      }
      break;
    case 1:
      op = (isa == rv32) ? rv_op_c_jal : rv_op_c_addiw;
      break;
    case 2:
      op = rv_op_c_li;
      break;
    case 3:
      switch (((inst >> 7) & 0b11111)) {
      case 2:
        op = rv_op_c_addi16sp;
        break;
      default:
        op = rv_op_c_lui;
        break;
      }
      break;
    case 4:
      switch (((inst >> 10) & 0b11)) {
      case 0:
        op = rv_op_c_srli;
        break;
      case 1:
        op = rv_op_c_srai;
        break;
      case 2:
        op = rv_op_c_andi;
        break;
      case 3:
        switch (((inst >> 10) & 0b100) | ((inst >> 5) & 0b011)) {
        case 0:
          op = rv_op_c_sub;
          break;
        case 1:
          op = rv_op_c_xor;
          break;
        case 2:
          op = rv_op_c_or;
          break;
        case 3:
          op = rv_op_c_and;
          break;
        case 4:
          op = rv_op_c_subw;
          break;
        case 5:
          op = rv_op_c_addw;
          break;
        }
        break;
      }
      break;
    case 5:
      op = rv_op_c_j;
      break;
    case 6:
      op = rv_op_c_beqz;
      break;
    case 7:
      op = rv_op_c_bnez;
      break;
    }
    break;
  case 2: // compression instruction instLen=16bits
    switch (((inst >> 13) & 0b111)) {
    case 0:
      op = rv_op_c_slli;
      break;
    case 1:
      op = (isa == rv128) ? rv_op_c_lqsp : rv_op_c_fldsp;
      break;
    case 2:
      op = rv_op_c_lwsp;
      break;
    case 3:
      op = (isa == rv32) ? rv_op_c_flwsp : rv_op_c_ldsp;
      break;
    case 4:
      switch (((inst >> 12) & 0b1)) {
      case 0:
        switch (((inst >> 2) & 0b11111)) {
        case 0:
          op = rv_op_c_jr;
          break;
        default:
          op = rv_op_c_mv;
          break;
        }
        break;
      case 1:
        switch (((inst >> 2) & 0b11111)) {
        case 0:
          switch (((inst >> 7) & 0b11111)) {
          case 0:
            op = rv_op_c_ebreak;
            break;
          default:
            op = rv_op_c_jalr;
            break;
          }
          break;
        default:
          op = rv_op_c_add;
          break;
        }
        break;
      }
      break;
    case 5:
      op = (isa == rv128) ? rv_op_c_sqsp : rv_op_c_fsdsp;
      break;
    case 6:
      op = rv_op_c_swsp;
      break;
    case 7:
      op = (isa == rv32) ? rv_op_c_fswsp : rv_op_c_sdsp;
      break;
    }
    break;
  case 3: // uncompression instruction instLen=32bits
    switch (((inst >> 2) & 0b11111)) {
    case 0:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_lb;
        break;
      case 1:
        op = rv_op_lh;
        break;
      case 2:
        op = rv_op_lw;
        break;
      case 3:
        op = rv_op_ld;
        break;
      case 4:
        op = rv_op_lbu;
        break;
      case 5:
        op = rv_op_lhu;
        break;
      case 6:
        op = rv_op_lwu;
        break;
      case 7:
        op = rv_op_ldu;
        break;
      }
      break;
    case 1:
      switch (((inst >> 12) & 0b111)) {
      case 2:
        op = rv_op_flw;
        break;
      case 3:
        op = rv_op_fld;
        break;
      case 4:
        op = rv_op_flq;
        break;
      }
      break;
    case 3:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_fence;
        break;
      case 1:
        op = rv_op_fence_i;
        break;
      case 2:
        op = rv_op_lq;
        break;
      }
      break;
    case 4:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_addi;
        break;
      case 1:
        switch (((inst >> 27) & 0b11111)) {
        case 0:
          op = rv_op_slli;
          break;
        }
        break;
      case 2:
        op = rv_op_slti;
        break;
      case 3:
        op = rv_op_sltiu;
        break;
      case 4:
        op = rv_op_xori;
        break;
      case 5:
        switch (((inst >> 27) & 0b11111)) {
        case 0:
          op = rv_op_srli;
          break;
        case 8:
          op = rv_op_srai;
          break;
        }
        break;
      case 6:
        op = rv_op_ori;
        break;
      case 7:
        op = rv_op_andi;
        break;
      }
      break;
    case 5:
      op = rv_op_auipc;
      break;
    case 6:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_addiw;
        break;
      case 1:
        switch (((inst >> 25) & 0b1111111)) {
        case 0:
          op = rv_op_slliw;
          break;
        }
        break;
      case 5:
        switch (((inst >> 25) & 0b1111111)) {
        case 0:
          op = rv_op_srliw;
          break;
        case 32:
          op = rv_op_sraiw;
          break;
        }
        break;
      }
      break;
    case 8:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_sb;
        break;
      case 1:
        op = rv_op_sh;
        break;
      case 2:
        op = rv_op_sw;
        break;
      case 3:
        op = rv_op_sd;
        break;
      case 4:
        op = rv_op_sq;
        break;
      }
      break;
    case 9:
      switch (((inst >> 12) & 0b111)) {
      case 2:
        op = rv_op_fsw;
        break;
      case 3:
        op = rv_op_fsd;
        break;
      case 4:
        op = rv_op_fsq;
        break;
      }
      break;
    case 11:
      switch (((inst >> 24) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
      case 2:
        op = rv_op_amoadd_w;
        break;
      case 3:
        op = rv_op_amoadd_d;
        break;
      case 4:
        op = rv_op_amoadd_q;
        break;
      case 10:
        op = rv_op_amoswap_w;
        break;
      case 11:
        op = rv_op_amoswap_d;
        break;
      case 12:
        op = rv_op_amoswap_q;
        break;
      case 18:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_lr_w;
          break;
        }
        break;
      case 19:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_lr_d;
          break;
        }
        break;
      case 20:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_lr_q;
          break;
        }
        break;
      case 26:
        op = rv_op_sc_w;
        break;
      case 27:
        op = rv_op_sc_d;
        break;
      case 28:
        op = rv_op_sc_q;
        break;
      case 34:
        op = rv_op_amoxor_w;
        break;
      case 35:
        op = rv_op_amoxor_d;
        break;
      case 36:
        op = rv_op_amoxor_q;
        break;
      case 66:
        op = rv_op_amoor_w;
        break;
      case 67:
        op = rv_op_amoor_d;
        break;
      case 68:
        op = rv_op_amoor_q;
        break;
      case 98:
        op = rv_op_amoand_w;
        break;
      case 99:
        op = rv_op_amoand_d;
        break;
      case 100:
        op = rv_op_amoand_q;
        break;
      case 130:
        op = rv_op_amomin_w;
        break;
      case 131:
        op = rv_op_amomin_d;
        break;
      case 132:
        op = rv_op_amomin_q;
        break;
      case 162:
        op = rv_op_amomax_w;
        break;
      case 163:
        op = rv_op_amomax_d;
        break;
      case 164:
        op = rv_op_amomax_q;
        break;
      case 194:
        op = rv_op_amominu_w;
        break;
      case 195:
        op = rv_op_amominu_d;
        break;
      case 196:
        op = rv_op_amominu_q;
        break;
      case 226:
        op = rv_op_amomaxu_w;
        break;
      case 227:
        op = rv_op_amomaxu_d;
        break;
      case 228:
        op = rv_op_amomaxu_q;
        break;
      }
      break;
    case 12:
      switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
      case 0:
        op = rv_op_add;
        break;
      case 1:
        op = rv_op_sll;
        break;
      case 2:
        op = rv_op_slt;
        break;
      case 3:
        op = rv_op_sltu;
        break;
      case 4:
        op = rv_op_xor;
        break;
      case 5:
        op = rv_op_srl;
        break;
      case 6:
        op = rv_op_or;
        break;
      case 7:
        op = rv_op_and;
        break;
      case 8:
        op = rv_op_mul;
        break;
      case 9:
        op = rv_op_mulh;
        break;
      case 10:
        op = rv_op_mulhsu;
        break;
      case 11:
        op = rv_op_mulhu;
        break;
      case 12:
        op = rv_op_div;
        break;
      case 13:
        op = rv_op_divu;
        break;
      case 14:
        op = rv_op_rem;
        break;
      case 15:
        op = rv_op_remu;
        break;
      case 256:
        op = rv_op_sub;
        break;
      case 261:
        op = rv_op_sra;
        break;
      }
      break;
    case 13:
      op = rv_op_lui;
      break;
    case 14:
      switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
      case 0:
        op = rv_op_addw;
        break;
      case 1:
        op = rv_op_sllw;
        break;
      case 5:
        op = rv_op_srlw;
        break;
      case 8:
        op = rv_op_mulw;
        break;
      case 12:
        op = rv_op_divw;
        break;
      case 13:
        op = rv_op_divuw;
        break;
      case 14:
        op = rv_op_remw;
        break;
      case 15:
        op = rv_op_remuw;
        break;
      case 256:
        op = rv_op_subw;
        break;
      case 261:
        op = rv_op_sraw;
        break;
      }
      break;
    case 16:
      switch (((inst >> 25) & 0b11)) {
      case 0:
        op = rv_op_fmadd_s;
        break;
      case 1:
        op = rv_op_fmadd_d;
        break;
      case 3:
        op = rv_op_fmadd_q;
        break;
      }
      break;
    case 17:
      switch (((inst >> 25) & 0b11)) {
      case 0:
        op = rv_op_fmsub_s;
        break;
      case 1:
        op = rv_op_fmsub_d;
        break;
      case 3:
        op = rv_op_fmsub_q;
        break;
      }
      break;
    case 18:
      switch (((inst >> 25) & 0b11)) {
      case 0:
        op = rv_op_fnmsub_s;
        break;
      case 1:
        op = rv_op_fnmsub_d;
        break;
      case 3:
        op = rv_op_fnmsub_q;
        break;
      }
      break;
    case 19:
      switch (((inst >> 25) & 0b11)) {
      case 0:
        op = rv_op_fnmadd_s;
        break;
      case 1:
        op = rv_op_fnmadd_d;
        break;
      case 3:
        op = rv_op_fnmadd_q;
        break;
      }
      break;
    case 20:
      switch (((inst >> 25) & 0b1111111)) {
      case 0:
        op = rv_op_fadd_s;
        break;
      case 1:
        op = rv_op_fadd_d;
        break;
      case 3:
        op = rv_op_fadd_q;
        break;
      case 4:
        op = rv_op_fsub_s;
        break;
      case 5:
        op = rv_op_fsub_d;
        break;
      case 7:
        op = rv_op_fsub_q;
        break;
      case 8:
        op = rv_op_fmul_s;
        break;
      case 9:
        op = rv_op_fmul_d;
        break;
      case 11:
        op = rv_op_fmul_q;
        break;
      case 12:
        op = rv_op_fdiv_s;
        break;
      case 13:
        op = rv_op_fdiv_d;
        break;
      case 15:
        op = rv_op_fdiv_q;
        break;
      case 16:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fsgnj_s;
          break;
        case 1:
          op = rv_op_fsgnjn_s;
          break;
        case 2:
          op = rv_op_fsgnjx_s;
          break;
        }
        break;
      case 17:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fsgnj_d;
          break;
        case 1:
          op = rv_op_fsgnjn_d;
          break;
        case 2:
          op = rv_op_fsgnjx_d;
          break;
        }
        break;
      case 19:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fsgnj_q;
          break;
        case 1:
          op = rv_op_fsgnjn_q;
          break;
        case 2:
          op = rv_op_fsgnjx_q;
          break;
        }
        break;
      case 20:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fmin_s;
          break;
        case 1:
          op = rv_op_fmax_s;
          break;
        }
        break;
      case 21:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fmin_d;
          break;
        case 1:
          op = rv_op_fmax_d;
          break;
        }
        break;
      case 23:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fmin_q;
          break;
        case 1:
          op = rv_op_fmax_q;
          break;
        }
        break;
      case 32:
        switch (((inst >> 20) & 0b11111)) {
        case 1:
          op = rv_op_fcvt_s_d;
          break;
        case 3:
          op = rv_op_fcvt_s_q;
          break;
        }
        break;
      case 33:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_d_s;
          break;
        case 3:
          op = rv_op_fcvt_d_q;
          break;
        }
        break;
      case 35:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_q_s;
          break;
        case 1:
          op = rv_op_fcvt_q_d;
          break;
        }
        break;
      case 44:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fsqrt_s;
          break;
        }
        break;
      case 45:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fsqrt_d;
          break;
        }
        break;
      case 47:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fsqrt_q;
          break;
        }
        break;
      case 80:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fle_s;
          break;
        case 1:
          op = rv_op_flt_s;
          break;
        case 2:
          op = rv_op_feq_s;
          break;
        }
        break;
      case 81:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fle_d;
          break;
        case 1:
          op = rv_op_flt_d;
          break;
        case 2:
          op = rv_op_feq_d;
          break;
        }
        break;
      case 83:
        switch (((inst >> 12) & 0b111)) {
        case 0:
          op = rv_op_fle_q;
          break;
        case 1:
          op = rv_op_flt_q;
          break;
        case 2:
          op = rv_op_feq_q;
          break;
        }
        break;
      case 96:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_w_s;
          break;
        case 1:
          op = rv_op_fcvt_wu_s;
          break;
        case 2:
          op = rv_op_fcvt_l_s;
          break;
        case 3:
          op = rv_op_fcvt_lu_s;
          break;
        }
        break;
      case 97:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_w_d;
          break;
        case 1:
          op = rv_op_fcvt_wu_d;
          break;
        case 2:
          op = rv_op_fcvt_l_d;
          break;
        case 3:
          op = rv_op_fcvt_lu_d;
          break;
        }
        break;
      case 99:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_w_q;
          break;
        case 1:
          op = rv_op_fcvt_wu_q;
          break;
        case 2:
          op = rv_op_fcvt_l_q;
          break;
        case 3:
          op = rv_op_fcvt_lu_q;
          break;
        }
        break;
      case 104:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_s_w;
          break;
        case 1:
          op = rv_op_fcvt_s_wu;
          break;
        case 2:
          op = rv_op_fcvt_s_l;
          break;
        case 3:
          op = rv_op_fcvt_s_lu;
          break;
        }
        break;
      case 105:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_d_w;
          break;
        case 1:
          op = rv_op_fcvt_d_wu;
          break;
        case 2:
          op = rv_op_fcvt_d_l;
          break;
        case 3:
          op = rv_op_fcvt_d_lu;
          break;
        }
        break;
      case 107:
        switch (((inst >> 20) & 0b11111)) {
        case 0:
          op = rv_op_fcvt_q_w;
          break;
        case 1:
          op = rv_op_fcvt_q_wu;
          break;
        case 2:
          op = rv_op_fcvt_q_l;
          break;
        case 3:
          op = rv_op_fcvt_q_lu;
          break;
        }
        break;
      case 112:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_x_s;
          break;
        case 1:
          op = rv_op_fclass_s;
          break;
        }
        break;
      case 113:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_x_d;
          break;
        case 1:
          op = rv_op_fclass_d;
          break;
        }
        break;
      case 115:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_x_q;
          break;
        case 1:
          op = rv_op_fclass_q;
          break;
        }
        break;
      case 120:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_s_x;
          break;
        }
        break;
      case 121:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_d_x;
          break;
        }
        break;
      case 123:
        switch (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
        case 0:
          op = rv_op_fmv_q_x;
          break;
        }
        break;
      }
      break;
    case 22:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_addid;
        break;
      case 1:
        switch (((inst >> 26) & 0b111111)) {
        case 0:
          op = rv_op_sllid;
          break;
        }
        break;
      case 5:
        switch (((inst >> 26) & 0b111111)) {
        case 0:
          op = rv_op_srlid;
          break;
        case 16:
          op = rv_op_sraid;
          break;
        }
        break;
      }
      break;
    case 24:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_beq;
        break;
      case 1:
        op = rv_op_bne;
        break;
      case 4:
        op = rv_op_blt;
        break;
      case 5:
        op = rv_op_bge;
        break;
      case 6:
        op = rv_op_bltu;
        break;
      case 7:
        op = rv_op_bgeu;
        break;
      }
      break;
    case 25:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        op = rv_op_jalr;
        break;
      }
      break;
    case 27:
      op = rv_op_jal;
      break;
    case 28:
      switch (((inst >> 12) & 0b111)) {
      case 0:
        switch (((inst >> 20) & 0b111111100000) |
                ((inst >> 7) & 0b000000011111)) {
        case 0:
          switch (((inst >> 15) & 0b1111111111)) {
          case 0:
            op = rv_op_ecall;
            break;
          case 32:
            op = rv_op_ebreak;
            break;
          case 64:
            op = rv_op_uret;
            break;
          }
          break;
        case 256:
          switch (((inst >> 20) & 0b11111)) {
          case 2:
            switch (((inst >> 15) & 0b11111)) {
            case 0:
              op = rv_op_sret;
              break;
            }
            break;
          case 4:
            op = rv_op_sfence_vm;
            break;
          case 5:
            switch (((inst >> 15) & 0b11111)) {
            case 0:
              op = rv_op_wfi;
              break;
            }
            break;
          }
          break;
        case 288:
          op = rv_op_sfence_vma;
          break;
        case 512:
          switch (((inst >> 15) & 0b1111111111)) {
          case 64:
            op = rv_op_hret;
            break;
          }
          break;
        case 768:
          switch (((inst >> 15) & 0b1111111111)) {
          case 64:
            op = rv_op_mret;
            break;
          }
          break;
        case 1952:
          switch (((inst >> 15) & 0b1111111111)) {
          case 576:
            op = rv_op_dret;
            break;
          }
          break;
        }
        break;
      case 1:
        op = rv_op_csrrw;
        break;
      case 2:
        op = rv_op_csrrs;
        break;
      case 3:
        op = rv_op_csrrc;
        break;
      case 5:
        op = rv_op_csrrwi;
        break;
      case 6:
        op = rv_op_csrrsi;
        break;
      case 7:
        op = rv_op_csrrci;
        break;
      }
      break;
    case 30:
      switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
      case 0:
        op = rv_op_addd;
        break;
      case 1:
        op = rv_op_slld;
        break;
      case 5:
        op = rv_op_srld;
        break;
      case 8:
        op = rv_op_muld;
        break;
      case 12:
        op = rv_op_divd;
        break;
      case 13:
        op = rv_op_divud;
        break;
      case 14:
        op = rv_op_remd;
        break;
      case 15:
        op = rv_op_remud;
        break;
      case 256:
        op = rv_op_subd;
        break;
      case 261:
        op = rv_op_srad;
        break;
      }
      break;
    }
    break;
  }
  instInfo->op = op;
}

/* operand extractors */

static uint32_t operand_rd(rv_inst inst) { return (inst << 52) >> 59; }

static uint32_t operand_rs1(rv_inst inst) { return (inst << 44) >> 59; }

static uint32_t operand_rs2(rv_inst inst) { return (inst << 39) >> 59; }

static uint32_t operand_rs3(rv_inst inst) { return (inst << 32) >> 59; }

static uint32_t operand_aq(rv_inst inst) { return (inst << 37) >> 63; }

static uint32_t operand_rl(rv_inst inst) { return (inst << 38) >> 63; }

static uint32_t operand_pred(rv_inst inst) { return (inst << 36) >> 60; }

static uint32_t operand_succ(rv_inst inst) { return (inst << 40) >> 60; }

static uint32_t operand_rm(rv_inst inst) { return (inst << 49) >> 61; }

static uint32_t operand_shamt5(rv_inst inst) { return (inst << 39) >> 59; }

static uint32_t operand_shamt6(rv_inst inst) { return (inst << 38) >> 58; }

static uint32_t operand_shamt7(rv_inst inst) { return (inst << 37) >> 57; }

static uint32_t operand_crdq(rv_inst inst) { return (inst << 59) >> 61; }

static uint32_t operand_crs1q(rv_inst inst) { return (inst << 54) >> 61; }

static uint32_t operand_crs1rdq(rv_inst inst) { return (inst << 54) >> 61; }

static uint32_t operand_crs2q(rv_inst inst) { return (inst << 59) >> 61; }

static uint32_t operand_crd(rv_inst inst) { return (inst << 52) >> 59; }

static uint32_t operand_crs1(rv_inst inst) { return (inst << 52) >> 59; }

static uint32_t operand_crs1rd(rv_inst inst) { return (inst << 52) >> 59; }

static uint32_t operand_crs2(rv_inst inst) { return (inst << 57) >> 59; }

static uint32_t operand_cimmsh5(rv_inst inst) { return (inst << 57) >> 59; }

static uint32_t operand_csr12(rv_inst inst) { return (inst << 32) >> 52; }

static int32_t operand_imm12(rv_inst inst) {
  return ((int64_t)inst << 32) >> 52;
}

static int32_t operand_imm20(rv_inst inst) {
  return (((int64_t)inst << 32) >> 44) << 12;
}

static int32_t operand_jimm20(rv_inst inst) {
  return (((int64_t)inst << 32) >> 63) << 20 | ((inst << 33) >> 54) << 1 |
         ((inst << 43) >> 63) << 11 | ((inst << 44) >> 56) << 12;
}

static int32_t operand_simm12(rv_inst inst) {
  return (((int64_t)inst << 32) >> 57) << 5 | (inst << 52) >> 59;
}

static int32_t operand_sbimm12(rv_inst inst) {
  return (((int64_t)inst << 32) >> 63) << 12 | ((inst << 33) >> 58) << 5 |
         ((inst << 52) >> 60) << 1 | ((inst << 56) >> 63) << 11;
}

static uint32_t operand_cimmsh6(rv_inst inst) {
  return ((inst << 51) >> 63) << 5 | (inst << 57) >> 59;
}

static int32_t operand_cimmi(rv_inst inst) {
  return (((int64_t)inst << 51) >> 63) << 5 | (inst << 57) >> 59;
}

static int32_t operand_cimmui(rv_inst inst) {
  return (((int64_t)inst << 51) >> 63) << 17 | ((inst << 57) >> 59) << 12;
}

static uint32_t operand_cimmlwsp(rv_inst inst) {
  return ((inst << 51) >> 63) << 5 | ((inst << 57) >> 61) << 2 |
         ((inst << 60) >> 62) << 6;
}

static uint32_t operand_cimmldsp(rv_inst inst) {
  return ((inst << 51) >> 63) << 5 | ((inst << 57) >> 62) << 3 |
         ((inst << 59) >> 61) << 6;
}

static uint32_t operand_cimmlqsp(rv_inst inst) {
  return ((inst << 51) >> 63) << 5 | ((inst << 57) >> 63) << 4 |
         ((inst << 58) >> 60) << 6;
}

static int32_t operand_cimm16sp(rv_inst inst) {
  return (((int64_t)inst << 51) >> 63) << 9 | ((inst << 57) >> 63) << 4 |
         ((inst << 58) >> 63) << 6 | ((inst << 59) >> 62) << 7 |
         ((inst << 61) >> 63) << 5;
}

static int32_t operand_cimmj(rv_inst inst) {
  return (((int64_t)inst << 51) >> 63) << 11 | ((inst << 52) >> 63) << 4 |
         ((inst << 53) >> 62) << 8 | ((inst << 55) >> 63) << 10 |
         ((inst << 56) >> 63) << 6 | ((inst << 57) >> 63) << 7 |
         ((inst << 58) >> 61) << 1 | ((inst << 61) >> 63) << 5;
}

static int32_t operand_cimmb(rv_inst inst) {
  return (((int64_t)inst << 51) >> 63) << 8 | ((inst << 52) >> 62) << 3 |
         ((inst << 57) >> 62) << 6 | ((inst << 59) >> 62) << 1 |
         ((inst << 61) >> 63) << 5;
}

static uint32_t operand_cimmswsp(rv_inst inst) {
  return ((inst << 51) >> 60) << 2 | ((inst << 55) >> 62) << 6;
}

static uint32_t operand_cimmsdsp(rv_inst inst) {
  return ((inst << 51) >> 61) << 3 | ((inst << 54) >> 61) << 6;
}

static uint32_t operand_cimmsqsp(rv_inst inst) {
  return ((inst << 51) >> 62) << 4 | ((inst << 53) >> 60) << 6;
}

static uint32_t operand_cimm4spn(rv_inst inst) {
  return ((inst << 51) >> 62) << 4 | ((inst << 53) >> 60) << 6 |
         ((inst << 57) >> 63) << 2 | ((inst << 58) >> 63) << 3;
}

static uint32_t operand_cimmw(rv_inst inst) {
  return ((inst << 51) >> 61) << 3 | ((inst << 57) >> 63) << 2 |
         ((inst << 58) >> 63) << 6;
}

static uint32_t operand_cimmd(rv_inst inst) {
  return ((inst << 51) >> 61) << 3 | ((inst << 57) >> 62) << 6;
}

static uint32_t operand_cimmq(rv_inst inst) {
  return ((inst << 51) >> 62) << 4 | ((inst << 53) >> 63) << 8 |
         ((inst << 57) >> 62) << 6;
}

/* decode operands */

static void rv_inst_oprands(rv_instInfo *instInfo) {
  rv_inst inst = instInfo->inst;
  instInfo->encodingType = opcode_data[instInfo->op].encodingType;
  switch (instInfo->encodingType) {
  case rv_encodingType_none:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    break;
  case rv_encodingType_u:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_imm20(inst);
    break;
  case rv_encodingType_uj:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_jimm20(inst);
    break;
  case rv_encodingType_i:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_imm12(inst);
    break;
  case rv_encodingType_i_sh5:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_shamt5(inst);
    break;
  case rv_encodingType_i_sh6:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_shamt6(inst);
    break;
  case rv_encodingType_i_sh7:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_shamt7(inst);
    break;
  case rv_encodingType_i_csr:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_csr12(inst);
    break;
  case rv_encodingType_s:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->imm = operand_simm12(inst);
    break;
  case rv_encodingType_sb:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->imm = operand_sbimm12(inst);
    break;
  case rv_encodingType_r:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->imm = 0;
    break;
  case rv_encodingType_r_m:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->imm = 0;
    instInfo->rm = operand_rm(inst);
    break;
  case rv_encodingType_r4_m:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->rs3 = operand_rs3(inst);
    instInfo->imm = 0;
    instInfo->rm = operand_rm(inst);
    break;
  case rv_encodingType_r_a:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = operand_rs2(inst);
    instInfo->imm = 0;
    instInfo->aq = operand_aq(inst);
    instInfo->rl = operand_rl(inst);
    break;
  case rv_encodingType_r_l:
    instInfo->rd = operand_rd(inst);
    instInfo->rs1 = operand_rs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    instInfo->aq = operand_aq(inst);
    instInfo->rl = operand_rl(inst);
    break;
  case rv_encodingType_r_f:
    instInfo->rd = instInfo->rs1 = instInfo->rs2 = reg_x00_f00;
    instInfo->pred = operand_pred(inst);
    instInfo->succ = operand_succ(inst);
    instInfo->imm = 0;
    break;
  case rv_encodingType_cb:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmb(inst);
    break;
  case rv_encodingType_cb_imm:
    instInfo->rd = instInfo->rs1 = operand_crs1rdq(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmi(inst);
    break;
  case rv_encodingType_cb_sh5:
    instInfo->rd = instInfo->rs1 = operand_crs1rdq(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmsh5(inst);
    break;
  case rv_encodingType_cb_sh6:
    instInfo->rd = instInfo->rs1 = operand_crs1rdq(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmsh6(inst);
    break;
  case rv_encodingType_ci:
    instInfo->rd = instInfo->rs1 = operand_crs1rd(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmi(inst);
    break;
  case rv_encodingType_ci_sh5:
    instInfo->rd = instInfo->rs1 = operand_crs1rd(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmsh5(inst);
    break;
  case rv_encodingType_ci_sh6:
    instInfo->rd = instInfo->rs1 = operand_crs1rd(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmsh6(inst);
    break;
  case rv_encodingType_ci_16sp:
    instInfo->rd = reg_x02_f02;
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimm16sp(inst);
    break;
  case rv_encodingType_ci_lwsp:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmlwsp(inst);
    break;
  case rv_encodingType_ci_ldsp:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmldsp(inst);
    break;
  case rv_encodingType_ci_lqsp:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmlqsp(inst);
    break;
  case rv_encodingType_ci_li:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmi(inst);
    break;
  case rv_encodingType_ci_lui:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmui(inst);
    break;
  case rv_encodingType_ci_none:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    break;
  case rv_encodingType_ciw_4spn:
    instInfo->rd = operand_crdq(inst) + 8;
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimm4spn(inst);
    break;
  case rv_encodingType_cj:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmj(inst);
    break;
  case rv_encodingType_cj_jal:
    instInfo->rd = reg_x01_f01;
    instInfo->rs1 = reg_x00_f00;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmj(inst);
    break;
  case rv_encodingType_cl_lw:
    instInfo->rd = operand_crdq(inst) + 8;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmw(inst);
    break;
  case rv_encodingType_cl_ld:
    instInfo->rd = operand_crdq(inst) + 8;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmd(inst);
    break;
  case rv_encodingType_cl_lq:
    instInfo->rd = operand_crdq(inst) + 8;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = operand_cimmq(inst);
    break;
  case rv_encodingType_cr:
    instInfo->rd = instInfo->rs1 = operand_crs1rd(inst);
    instInfo->rs2 = operand_crs2(inst);
    instInfo->imm = 0;
    break;
  case rv_encodingType_cr_mv:
    instInfo->rd = operand_crd(inst);
    instInfo->rs1 = operand_crs2(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    break;
  case rv_encodingType_cr_jalr:
    instInfo->rd = reg_x01_f01;
    instInfo->rs1 = operand_crs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    break;
  case rv_encodingType_cr_jr:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_crs1(inst);
    instInfo->rs2 = reg_x00_f00;
    instInfo->imm = 0;
    break;
  case rv_encodingType_cs:
    instInfo->rd = instInfo->rs1 = operand_crs1rdq(inst) + 8;
    instInfo->rs2 = operand_crs2q(inst) + 8;
    instInfo->imm = 0;
    break;
  case rv_encodingType_cs_sw:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = operand_crs2q(inst) + 8;
    instInfo->imm = operand_cimmw(inst);
    break;
  case rv_encodingType_cs_sd:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = operand_crs2q(inst) + 8;
    instInfo->imm = operand_cimmd(inst);
    break;
  case rv_encodingType_cs_sq:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = operand_crs1q(inst) + 8;
    instInfo->rs2 = operand_crs2q(inst) + 8;
    instInfo->imm = operand_cimmq(inst);
    break;
  case rv_encodingType_css_swsp:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = operand_crs2(inst);
    instInfo->imm = operand_cimmswsp(inst);
    break;
  case rv_encodingType_css_sdsp:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = operand_crs2(inst);
    instInfo->imm = operand_cimmsdsp(inst);
    break;
  case rv_encodingType_css_sqsp:
    instInfo->rd = reg_x00_f00;
    instInfo->rs1 = reg_x02_f02;
    instInfo->rs2 = operand_crs2(inst);
    instInfo->imm = operand_cimmsqsp(inst);
    break;
  };
}

/* decompress instruction */

static void rv_inst_decompress(rv_instInfo *instInfo, rv_isa isa) {
  int decomp_op;
  switch (isa) {
  case rv32:
    decomp_op = opcode_data[instInfo->op].decomp_rv32;
    break;
  case rv64:
    decomp_op = opcode_data[instInfo->op].decomp_rv64;
    break;
  case rv128:
    decomp_op = opcode_data[instInfo->op].decomp_rv128;
    break;
  }
  if (decomp_op != rv_op_illegal) {
    if ((opcode_data[instInfo->op].decomp_data & rvcd_imm_nz) &&
        instInfo->imm == 0) {
      instInfo->op = rv_op_illegal;
    } else {
      instInfo->op = decomp_op;
      instInfo->encodingType = opcode_data[decomp_op].encodingType;
    }
  }
}

/* check constraint */

static bool check_constraints(rv_instInfo *instInfo, const rvc_constraint *c) {
  int32_t imm = instInfo->imm;
  uint8_t rd = instInfo->rd;
  uint8_t rs1 = instInfo->rs1;
  uint8_t rs2 = instInfo->rs2;

  while (*c != rvc_end) {
    switch (*c) {
    case rvc_rd_eq_ra:
      if (!(rd == 1))
        return false;
      break;
    case rvc_rd_eq_x0:
      if (!(rd == 0))
        return false;
      break;
    case rvc_rs1_eq_x0:
      if (!(rs1 == 0))
        return false;
      break;
    case rvc_rs2_eq_x0:
      if (!(rs2 == 0))
        return false;
      break;
    case rvc_rs2_eq_rs1:
      if (!(rs2 == rs1))
        return false;
      break;
    case rvc_rs1_eq_ra:
      if (!(rs1 == 1))
        return false;
      break;
    case rvc_imm_eq_zero:
      if (!(imm == 0))
        return false;
      break;
    case rvc_imm_eq_n1:
      if (!(imm == -1))
        return false;
      break;
    case rvc_imm_eq_p1:
      if (!(imm == 1))
        return false;
      break;
    case rvc_csr_eq_0x001:
      if (!(imm == 0x001))
        return false;
      break;
    case rvc_csr_eq_0x002:
      if (!(imm == 0x002))
        return false;
      break;
    case rvc_csr_eq_0x003:
      if (!(imm == 0x003))
        return false;
      break;
    case rvc_csr_eq_0xc00:
      if (!(imm == 0xc00))
        return false;
      break;
    case rvc_csr_eq_0xc01:
      if (!(imm == 0xc01))
        return false;
      break;
    case rvc_csr_eq_0xc02:
      if (!(imm == 0xc02))
        return false;
      break;
    case rvc_csr_eq_0xc80:
      if (!(imm == 0xc80))
        return false;
      break;
    case rvc_csr_eq_0xc81:
      if (!(imm == 0xc81))
        return false;
      break;
    case rvc_csr_eq_0xc82:
      if (!(imm == 0xc82))
        return false;
      break;
    default:
      break;
    }
    c++;
  }
  return true;
}

/* lift instruction to pseudo-instruction */

static void rv_inst_pseudo(rv_instInfo *instInfo) {
  const rv_compData *comp_data = opcode_data[instInfo->op].pseudo;
  if (!comp_data) {
    return;
  }
  while (comp_data->constraints) {
    if (check_constraints(instInfo, comp_data->constraints)) {
      instInfo->op = comp_data->op;
      instInfo->encodingType = opcode_data[instInfo->op].encodingType;
      return;
    }
    comp_data++;
  }
}

/* format instruction */

static void append(char *s1, const char *s2, ssize_t n) {
  ssize_t l1 = strlen(s1);
  if (n - l1 - 1 > 0) {
    strncat(s1, s2, n - l1);
  }
}

#define INST_LEN_2 ("%04h" PRIx16 "              ")
#define INST_LEN_4 ("%08" PRIx32 "          ")
#define INST_LEN_6 ("%012" PRIx64 "      ")
#define INST_LEN_8 ("%016" PRIx64 "  ")

static void rv_inst_format(char *buf, size_t buflen, size_t tab,
                           rv_instInfo *instInfo) {
  char tmp[64];
  const char *fmt;

  size_t len = inst_length(instInfo->inst);
  switch (len) {
  case 2:
    snprintf(buf, buflen, INST_LEN_2, (int16_t)instInfo->inst);
    break;
  case 4:
    snprintf(buf, buflen, INST_LEN_4, (int32_t)instInfo->inst);
    break;
  case 6:
    snprintf(buf, buflen, INST_LEN_6, instInfo->inst);
    break;
  default:
    snprintf(buf, buflen, INST_LEN_8, (int64_t)instInfo->inst);
    break;
  }

  fmt = opcode_data[instInfo->op].format;
  while (*fmt) {
    switch (*fmt) {
    case 'O':
      append(buf, opcode_data[instInfo->op].name, buflen);
      break;
    case '(':
      append(buf, "(", buflen);
      break;
    case ',':
      append(buf, ",", buflen);
      break;
    case ')':
      append(buf, ")", buflen);
      break;
    case '0':
      append(buf, rv_ireg_name_sym[instInfo->rd], buflen);
      break;
    case '1':
      append(buf, rv_ireg_name_sym[instInfo->rs1], buflen);
      break;
    case '2':
      append(buf, rv_ireg_name_sym[instInfo->rs2], buflen);
      break;
    case '3':
      append(buf, rv_freg_name_sym[instInfo->rd], buflen);
      break;
    case '4':
      append(buf, rv_freg_name_sym[instInfo->rs1], buflen);
      break;
    case '5':
      append(buf, rv_freg_name_sym[instInfo->rs2], buflen);
      break;
    case '6':
      append(buf, rv_freg_name_sym[instInfo->rs3], buflen);
      break;
    case '7':
      snprintf(tmp, sizeof(tmp), "%d", instInfo->rs1);
      append(buf, tmp, buflen);
      break;
    case 'i':
      snprintf(tmp, sizeof(tmp), "%d", instInfo->imm);
      append(buf, tmp, buflen);
      break;
    case 'o':
      snprintf(tmp, sizeof(tmp), "%d", instInfo->imm);
      append(buf, tmp, buflen);
      while (strlen(buf) < tab * 2) {
        append(buf, " ", buflen);
      }
      snprintf(tmp, sizeof(tmp), "# 0x%" PRIx64, instInfo->pc + instInfo->imm);
      append(buf, tmp, buflen);
      break;
    case 'c': {
      const char *name = csr_name(instInfo->imm & 0xfff);
      if (name) {
        append(buf, name, buflen);
      } else {
        snprintf(tmp, sizeof(tmp), "0x%03x", instInfo->imm & 0xfff);
        append(buf, tmp, buflen);
      }
      break;
    }
    case 'r':
      switch (instInfo->rm) {
      case rv_rm_rne:
        append(buf, "rne", buflen);
        break;
      case rv_rm_rtz:
        append(buf, "rtz", buflen);
        break;
      case rv_rm_rdn:
        append(buf, "rdn", buflen);
        break;
      case rv_rm_rup:
        append(buf, "rup", buflen);
        break;
      case rv_rm_rmm:
        append(buf, "rmm", buflen);
        break;
      case rv_rm_dyn:
        append(buf, "dyn", buflen);
        break;
      default:
        append(buf, "inv", buflen);
        break;
      }
      break;
    case 'p':
      if (instInfo->pred & rv_fence_i) {
        append(buf, "i", buflen);
      }
      if (instInfo->pred & rv_fence_o) {
        append(buf, "o", buflen);
      }
      if (instInfo->pred & rv_fence_r) {
        append(buf, "r", buflen);
      }
      if (instInfo->pred & rv_fence_w) {
        append(buf, "w", buflen);
      }
      break;
    case 's':
      if (instInfo->succ & rv_fence_i) {
        append(buf, "i", buflen);
      }
      if (instInfo->succ & rv_fence_o) {
        append(buf, "o", buflen);
      }
      if (instInfo->succ & rv_fence_r) {
        append(buf, "r", buflen);
      }
      if (instInfo->succ & rv_fence_w) {
        append(buf, "w", buflen);
      }
      break;
    case '\t':
      while (strlen(buf) < tab) {
        append(buf, " ", buflen);
      }
      break;
    case 'A':
      if (instInfo->aq) {
        append(buf, ".aq", buflen);
      }
      break;
    case 'R':
      if (instInfo->rl) {
        append(buf, ".rl", buflen);
      }
      break;
    default:
      break;
    }
    fmt++;
  }
}

/* instruction fetch */

void inst_fetch(const uint8_t *data, rv_inst *instp, size_t *length) {
  rv_inst inst = ((rv_inst)data[1] << 8) | ((rv_inst)data[0]);
  size_t len = *length = inst_length(inst);
  if (len >= 8)
    inst |= ((rv_inst)data[7] << 56) | ((rv_inst)data[6] << 48);
  if (len >= 6)
    inst |= ((rv_inst)data[5] << 40) | ((rv_inst)data[4] << 32);
  if (len >= 4)
    inst |= ((rv_inst)data[3] << 24) | ((rv_inst)data[2] << 16);
  *instp = inst;
}

/* disassemble instruction */

static void disassemble_inst(uint8_t *buf, size_t buflen, rv_isa isa,
                             rv_inst inst, uint64_t pc) {
  rv_instInfo dec = {.pc = pc, .inst = inst};
  rv_inst_opcode(&dec, isa);
  rv_inst_oprands(&dec);
  // Whether to display compression instructions?
  // rv_inst_decompress(&dec, isa);
  rv_inst_pseudo(&dec);
  rv_inst_format(buf, buflen, 32, &dec);
  fprintf(stderr, "%04" PRIx64 ":  %s\n", pc, buf);
}

int disassemble_text_section(const uint8_t *inputFileName) {
  int32_t inst;

  FILE *fileHandle = fopen(inputFileName, "rb");

  fprintf(stderr, "\n\n");
  fprintf(stderr, "Disassembly of section .text:\n");

  if (!shdrTextOff) {
    fprintf(stderr, "No .text section\n");
    return 0;
  }

  uint8_t *instBuffer = malloc(shdrTextSize);
  uint8_t *instBufferEnd = instBuffer + shdrTextSize;

  if (!fseek(fileHandle, shdrTextOff, SEEK_SET))
    fread(instBuffer, 1, shdrTextSize, fileHandle);

  while (instBuffer != instBufferEnd) {
    uint8_t buf[80] = {0};

    inst = byte_get_little_endian(instBuffer, uncompressionInstLen);

    if (inst_length(inst) == 4) { // uncompression instruction length is 4
      disassemble_inst(buf, sizeof(buf), riscvLen == 64 ? rv64 : rv32, inst,
                       shdrTextOff);
      instBuffer += uncompressionInstLen;
      shdrTextOff += uncompressionInstLen;
    } else if (inst_length(inst) == 2) { // compression instruction length is 2
      inst = byte_get_little_endian(instBuffer, compressionInstLen);
      disassemble_inst(buf, sizeof(buf), riscvLen == 64 ? rv64 : rv32, inst,
                       shdrTextOff);
      instBuffer += compressionInstLen;
      shdrTextOff += compressionInstLen;
    } else { // Other instruction lengths are not supported temporarily
      fprintf(stderr, "instruction length is error!\n\n");
    }
  }

  close_file(fileHandle);

  fprintf(stderr, "\n\n");
  return 0;
}
