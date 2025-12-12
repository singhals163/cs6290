// --------------------------------------------------------------------- //
// You will need to modify this file.                                    //
// You may add any code you need, as long as you correctly implement the //
// required pipe_cycle_*() functions already listed in this file.        //
// In part B, you will also need to implement pipe_check_bpred().        //
// --------------------------------------------------------------------- //

// pipeline.cpp
// Implements functions to simulate a pipelined processor.

#include "pipeline.h"
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

#define ins(stage, i) p->pipe_latch[stage][i]

/**
 * Read a single trace record from the trace file and use it to populate the
 * given fetch_op.
 * 
 * You should not modify this function.
 * 
 * @param p the pipeline whose trace file should be read
 * @param fetch_op the PipelineLatch struct to populate
 */
void pipe_get_fetch_op(Pipeline *p, PipelineLatch *fetch_op)
{
    TraceRec *trace_rec = &fetch_op->trace_rec;
    uint8_t *trace_rec_buf = (uint8_t *)trace_rec;
    size_t bytes_read_total = 0;
    ssize_t bytes_read_last = 0;
    size_t bytes_left = sizeof(*trace_rec);

    // Read a total of sizeof(TraceRec) bytes from the trace file.
    while (bytes_left > 0)
    {
        bytes_read_last = read(p->trace_fd, trace_rec_buf, bytes_left);
        if (bytes_read_last <= 0)
        {
            // EOF or error
            break;
        }

        trace_rec_buf += bytes_read_last;
        bytes_read_total += bytes_read_last;
        bytes_left -= bytes_read_last;
    }

    // Check for error conditions.
    if (bytes_left > 0 || trace_rec->op_type >= NUM_OP_TYPES)
    {
        fetch_op->valid = false;
        p->halt_op_id = p->last_op_id;

        if (p->last_op_id == 0)
        {
            p->halt = true;
        }

        if (bytes_read_last == -1)
        {
            fprintf(stderr, "\n");
            perror("Couldn't read from pipe");
            return;
        }

        if (bytes_read_total == 0)
        {
            // No more trace records to read
            return;
        }

        // Too few bytes read or invalid op_type
        fprintf(stderr, "\n");
        fprintf(stderr, "Error: Invalid trace file\n");
        return;
    }

    // Got a valid trace record!
    fetch_op->valid = true;
    fetch_op->stall = false;
    fetch_op->is_mispred_cbr = false;
    fetch_op->op_id = ++p->last_op_id;
}

/**
 * Allocate and initialize a new pipeline.
 * 
 * You should not need to modify this function.
 * 
 * @param trace_fd the file descriptor from which to read trace records
 * @return a pointer to a newly allocated pipeline
 */
Pipeline *pipe_init(int trace_fd)
{
    printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

    // Allocate pipeline.
    Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));

    // Initialize pipeline.
    p->trace_fd = trace_fd;
    p->halt_op_id = (uint64_t)(-1) - 3;

    // Allocate and initialize a branch predictor if needed.
    if (BPRED_POLICY != BPRED_PERFECT)
    {
        p->b_pred = new BPred(BPRED_POLICY);
    }

    return p;
}

/**
 * Print out the state of the pipeline latches for debugging purposes.
 * 
 * You may use this function to help debug your pipeline implementation, but
 * please remove calls to this function before submitting the lab.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p)
{
    printf("\n");
    // Print table header
    for (uint8_t latch_type = 0; latch_type < NUM_LATCH_TYPES; latch_type++)
    {
        switch (latch_type)
        {
        case IF_LATCH:
            printf(" IF:    ");
            break;
        case ID_LATCH:
            printf(" ID:    ");
            break;
        case EX_LATCH:
            printf(" EX:    ");
            break;
        case MA_LATCH:
            printf(" MA:    ");
            break;
        default:
            printf(" ------ ");
        }
    }
    printf("\n");

    // Print row for each lane in pipeline width
    for (uint8_t i = 0; i < PIPE_WIDTH; i++)
    {
        for (uint8_t latch_type = 0; latch_type < NUM_LATCH_TYPES;
             latch_type++)
        {
            if (p->pipe_latch[latch_type][i].valid)
            {
                printf(" %6lu ",
                       (unsigned long)p->pipe_latch[latch_type][i].op_id);
            }
            else
            {
                printf(" ------ ");
            }
        }
        printf("\n");
    }
    for (uint8_t i = 0; i < PIPE_WIDTH; i++)
    {
        for (uint8_t latch_type = 0; latch_type < NUM_LATCH_TYPES;
             latch_type++)
        {
            if (p->pipe_latch[latch_type][i].valid)
            {
                int dest = (p->pipe_latch[latch_type][i].trace_rec.dest_needed) ? 
                        p->pipe_latch[latch_type][i].trace_rec.dest_reg : -1;
                int src1 = (p->pipe_latch[latch_type][i].trace_rec.src1_needed) ? 
                        p->pipe_latch[latch_type][i].trace_rec.src1_reg : -1;
                int src2 = (p->pipe_latch[latch_type][i].trace_rec.src2_needed) ? 
                        p->pipe_latch[latch_type][i].trace_rec.src2_reg : -1;
                int cc_read = p->pipe_latch[latch_type][i].trace_rec.cc_read;
                int cc_write = p->pipe_latch[latch_type][i].trace_rec.cc_write;
                int br_dir = p->pipe_latch[latch_type][i].trace_rec.br_dir;

                const char *op_type;
                if (p->pipe_latch[latch_type][i].trace_rec.op_type == OP_ALU)
                    op_type = "ALU";
                else if (p->pipe_latch[latch_type][i].trace_rec.op_type == OP_LD)
                    op_type = "LD";
                else if (p->pipe_latch[latch_type][i].trace_rec.op_type == OP_ST)
                    op_type = "ST";
                else if (p->pipe_latch[latch_type][i].trace_rec.op_type == OP_CBR)
                    op_type = "BR";
                else
                    op_type = "OTHER";

                printf("(%lu : %s) dest: %d, src1: %d, src2: %d , ccread: %d, ccwrite: %d, br_dir: %d\n",
                       (unsigned long)p->pipe_latch[latch_type][i].op_id,
                       op_type,
                       dest,
                       src1,
                       src2,
                       cc_read,
                       cc_write,
                       br_dir);
            }
            else
            {
                printf(" ------ \n");
            }
        }
        printf("\n");
    }
}

/**
 * Simulate one cycle of all stages of a pipeline.
 * 
 * You should not need to modify this function except for debugging purposes.
 * If you add code to print debug output in this function, remove it or comment
 * it out before you submit the lab.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle(Pipeline *p)
{
    p->stat_num_cycle++;

    #ifdef DEBUG
        printf("\n--------------------------------------------\n");
        printf("Cycle count: %lu, retired instructions: %lu\n\n",
            (unsigned long)p->stat_num_cycle,
            (unsigned long)p->stat_retired_inst);
    #endif

    // In hardware, all pipeline stages execute in parallel, and each pipeline
    // latch is populated at the start of the next clock cycle.

    // In our simulator, we simulate the pipeline stages one at a time in
    // reverse order, from the Write Back stage (WB) to the Fetch stage (IF).
    // We do this so that each stage can read from the latch before it and
    // write to the latch after it without needing to "double-buffer" the
    // latches.

    // Additionally, it means that earlier pipeline stages can know about
    // stalls triggered in later pipeline stages in the same cycle, as would be
    // the case with hardware stall signals asserted by combinational logic.

    pipe_cycle_WB(p);
    pipe_cycle_MA(p);
    pipe_cycle_EX(p);
    pipe_cycle_ID(p);
    pipe_cycle_IF(p);

    // Compile with "make debug" to have this show!
    #ifdef DEBUG
        pipe_print_state(p);
    #endif
}

/**
 * Simulate one cycle of the Write Back stage (WB) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_WB(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->pipe_latch[MA_LATCH][i].valid)
        {
            p->stat_retired_inst++;

            if (p->pipe_latch[MA_LATCH][i].op_id >= p->halt_op_id)
            {
                // Halt the pipeline if we've reached the end of the trace.
                p->halt = true;
            }
            if(p->b_pred && p->b_pred->op_stall_id == ins(MA_LATCH, i).op_id) {
                p->fetch_cbr_stall = false;
            }
        }
    }
}

/**
 * Simulate one cycle of the Memory Access stage (MA) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_MA(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Copy each instruction from the EX latch to the MA latch.
        p->pipe_latch[MA_LATCH][i] = p->pipe_latch[EX_LATCH][i];
    }
}

/**
 * Simulate one cycle of the Execute stage (EX) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_EX(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Copy each instruction from the ID latch to the EX latch.
        p->pipe_latch[EX_LATCH][i] = p->pipe_latch[ID_LATCH][i];
    }
}

// RAW dependency
int isRawDependency(Pipeline *p, unsigned int i)
{
    TraceRec *src_trace_rec = &ins(IF_LATCH, i).trace_rec;
    int src1_reg = (src_trace_rec->src1_needed) ? src_trace_rec->src1_reg : -1;
    int src2_reg = (src_trace_rec->src2_needed) ? src_trace_rec->src2_reg : -1;
    int cc_rd = (src_trace_rec->cc_read) ? src_trace_rec->cc_read : -1;

    if (!(src1_reg != -1 || src2_reg != -1 || cc_rd != -1))
        return -1;

    int stall = 0;

    // run the loop on the stage of the pipeline in the order: ID_LATCH, EX_LATCH, MA_LATCH
    // in case there is any dependency found, set stall to true and handle accordingly

    // ID_LATCH
    for (unsigned int j = 0; j < PIPE_WIDTH; j++)
    {
        TraceRec *dest_trace_rec = &ins(IF_LATCH, j).trace_rec;
        if (j == i)
            continue;
        if (ins(IF_LATCH, j).valid == false)
            continue;
        int dest_reg_id = -1, cc_wr_id = -1;

        if (ins(IF_LATCH, j).op_id < ins(IF_LATCH, i).op_id)
        {
            dest_reg_id = (dest_trace_rec->dest_needed) ? dest_trace_rec->dest_reg : -1;
            cc_wr_id = (dest_trace_rec->cc_write) ? dest_trace_rec->cc_write : -1;
            if (dest_reg_id != -1 && (dest_reg_id == src1_reg || dest_reg_id == src2_reg))
            {
                stall = 1;
            }
            if (cc_wr_id == 1 && cc_rd == 1)
            {
                stall = 1;
            }
        }
    }
    if (stall)
        return stall;

    // EX_LATCH
    uint64_t max_op_id = 0;
    for (unsigned int j = 0; j < PIPE_WIDTH; j++)
    {
        TraceRec *dest_trace_rec = &ins(EX_LATCH, j).trace_rec;
        if (ins(EX_LATCH, j).valid == false)
            continue;
        int dest_reg_ex = (dest_trace_rec->dest_needed) ? dest_trace_rec->dest_reg : -1;
        int cc_wr_ex = (dest_trace_rec->cc_write) ? dest_trace_rec->cc_write : -1;
        uint64_t op_id_ex = ins(EX_LATCH, j).op_id;
        if ((dest_reg_ex != -1 && (dest_reg_ex == src1_reg || dest_reg_ex == src2_reg)) || (cc_wr_ex == 1 && cc_rd == 1))
        {
            if(op_id_ex > max_op_id) 
                max_op_id = op_id_ex;
            else continue;
            if (!ENABLE_EXE_FWD)
            {
                stall = 1;
            }
            else if (dest_trace_rec->op_type == OP_LD)
            {
                stall = 1;
            }
            else
                stall = 2;
        }
    }
    if (stall == 1)
        return stall;
    else if (stall == 2)
        return 0;

    // MA_LATCH
    for (unsigned int j = 0; j < PIPE_WIDTH; j++)
    {
        TraceRec *dest_trace_rec = &ins(MA_LATCH, j).trace_rec;
        if (ins(MA_LATCH, j).valid == false)
            continue;
        int dest_reg_ma = (dest_trace_rec->dest_needed) ? dest_trace_rec->dest_reg : -1;
        int cc_wr_ma = (dest_trace_rec->cc_write) ? dest_trace_rec->cc_write : -1;

        if ((dest_reg_ma != -1 && (dest_reg_ma == src1_reg || dest_reg_ma == src2_reg)) || (cc_wr_ma == 1 && cc_rd == 1))
        {
            if (!ENABLE_MEM_FWD)
            {
                stall = 1;
            }
        }
    }

    return stall;
}

/**
 * Simulate one cycle of the Instruction Decode stage (ID) of a pipeline.
 *
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 *
 * @param p the pipeline to simulate
 */
void pipe_cycle_ID(Pipeline *p)
{
    int *arr = (int *)calloc(PIPE_WIDTH, sizeof(int));
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        ins(ID_LATCH, i) = ins(IF_LATCH, i);
        ins(IF_LATCH, i).stall = false;
    }
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (ins(IF_LATCH, i).valid)
        {
            int stall = isRawDependency(p, i);

            // not a read instruction, so no RAW dependency
            if (stall == -1)
                continue;

            if (stall)
            {
                ins(ID_LATCH, i).valid = false;
                ins(IF_LATCH, i).stall = true;
                arr[i] = 1;
            }
            else
            {
                ins(ID_LATCH, i).valid = true;
                ins(IF_LATCH, i).stall = false;
            }
        }
    }

    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (ins(IF_LATCH, i).valid && !ins(IF_LATCH, i).stall)
        {
            for (unsigned int j = 0; j < PIPE_WIDTH; j++)
            {
                if (j == i)
                    continue;
                if (ins(IF_LATCH, j).valid && ins(IF_LATCH, j).stall && ins(IF_LATCH, j).op_id < ins(IF_LATCH, i).op_id)
                {
                    ins(ID_LATCH, i).valid = false;
                    ins(IF_LATCH, i).stall = true;
                    break;
                }
            }
        }
    }
}

/**
 * Simulate one cycle of the Instruction Fetch stage (IF) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_IF(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        #ifdef DEBUG
        printf("%d %d\n", p->fetch_cbr_stall, ins(IF_LATCH, i).stall);
        #endif
        if(p->fetch_cbr_stall && !ins(IF_LATCH, i).stall) {
            p->pipe_latch[IF_LATCH][i].valid = false;
        } else if (!p->pipe_latch[IF_LATCH][i].stall) {
            // Read an instruction from the trace file.
            PipelineLatch fetch_op;
            pipe_get_fetch_op(p, &fetch_op);

            // Handle branch (mis)prediction.
            if (BPRED_POLICY != BPRED_PERFECT)
            {
                pipe_check_bpred(p, &fetch_op);
            }

            // Copy the instruction to the IF latch.
            p->pipe_latch[IF_LATCH][i] = fetch_op;
        }
    }
}

/**
 * If the instruction just fetched is a conditional branch, check for a branch
 * misprediction, update the branch predictor, and set appropriate flags in the
 * pipeline.
 * 
 * You must implement this function in part B of the lab.
 * 
 * @param p the pipeline
 * @param fetch_op the pipeline latch containing the operation fetched
 */
void pipe_check_bpred(Pipeline *p, PipelineLatch *fetch_op)
{
    // TODO: For a conditional branch instruction, get a prediction from the
    // branch predictor.
    if(fetch_op->trace_rec.op_type != OP_CBR) {
        return;
    }
    BranchDirection br_dir = (BranchDirection)fetch_op->trace_rec.br_dir;
    BranchDirection br_dir_pred = p->b_pred->predict(fetch_op->trace_rec.inst_addr);
    // TODO: If the branch predictor mispredicted, mark the fetch_op
    // accordingly.
    #ifdef DEBUG
    printf("pred: %d, actual: %d\n", br_dir_pred, br_dir);
    #endif
    
    if(br_dir != br_dir_pred) {
        fetch_op->is_mispred_cbr = true;
        p->b_pred->op_stall_id = fetch_op->op_id;
    }
    // TODO: Immediately update the branch predictor.
    p->b_pred->update(fetch_op->trace_rec.inst_addr, br_dir_pred, br_dir);

    // TODO: If needed, stall the IF stage by setting the flag
    // p->fetch_cbr_stall.
    if(fetch_op->is_mispred_cbr) {
        p->fetch_cbr_stall = true;
    }
}
