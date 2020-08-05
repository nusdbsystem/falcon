### MPC

MPC allows parties to compute a function over their inputs while keeping the inputs
private. We use the additive secret sharing scheme (ASSS) based on
[SPDZ](https://github.com/data61/MP-SPDZ) for MPC. 

A value `x` is additively secretly shared among parties such that each party owns
a random share of `x`, such that the sum of these shares equals to `x`. Given ASSS
values, almost all the secure computations are supported in SPDZ, including secure
addition, secure multiplication, secure comparison, secure division, secure 
exponential, secure logarithm, and so on. 

In this project, we use the SPDZ project as a library, such that we can send inputs
to and receive outputs from the SPDZ engine. The required MPC computations are 
implemented in SPDZ by writing .mpc programs. 

In the executor, we define the following functions for communicating with SPDZ 
engine.

 * setup connections. Since SPDZ program is run in a decentralized way, the executors
 need to connect to SPDZ engine before sending inputs and receive outputs.
 * send public inputs. The APIs for public inputs and private inputs are different
 in SPDZ, thus, we differentiate the two cases.
 * send private inputs. In this case, a value `x` that is sent to SPDZ engine will
 be split into shares, such that each party receives a share.
 * receive outputs. After MPC computations, the result (either public or private)
 is sent back to the parties.

By default, the security parameter in ASSS is set to 128 bits.