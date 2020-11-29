// Author: Cornell University
//
// Module Name :    nic_defs
// Project :        F-NIC
// Description :    General definitions
//

`ifndef GEN_DEFS_VH_
`define GEN_DEFS_VH_

// IPv4 address structure, internal representation
//----------------------------------------------------------------------
typedef struct packed {
    logic [7:0] b0;
    logic [7:0] b1;
    logic [7:0] b2;
    logic [7:0] b3;
} IPv4;

// Port structure, internal representation
//----------------------------------------------------------------------
typedef logic[15:0] Port;


`endif //  GEN_DEFS_VH_