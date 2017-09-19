# Signal Composer

## Architecture

Here is a quick picture about the signaling architecture :

![GlobalArchitecture]

Key here are on both layer, **low** and **high**.

- **Low level** are in charge of handle a data exchange protocol to
 decode/encode and retransmit with an AGL compatible format, most convenient
 way using **Application Framework** event. These are divised in two parts,
 which are :
  - Transport Layer plug-in that read/write 1 protocol.
  - Decoding/Encoding part that expose signals and a way to access them.
- **High level signal composer** gathers multiple **low level** signaling
 sources and creates new virtuals signals from the **raw** signals defined (eg.
 signal made from gps latitude and longitude that calcul the heading of
 vehicle). It is modular and each signal source should be handled by specific
 plugins which take care of get the underlying event from **low level** or
 define signaling composition with simple or complex operation to output value
 from **raw** signals

There are three main parts with **Signal Composer**:

- Configuration files which could be splitted in differents files. That will
 define:
  - metadata with name of **signal composer** api name
  - additionnals configurations files
  - plugins used if so, **low level** signals sources
  - signals definitions
- Dedicated plugins
- Signal composer API

## Terminology

Here is a little terminology guide to set the vocabulary:

- **sources**
- **signals**
- **api**
- **event**
- **callbacks**
- **action**
- **virtual signals**
- **raw signals**

[GlobalArchitecture]: pictures/Global_Signaling_Architecture.png "Global architecture"
