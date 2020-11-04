-- -*- mode: haskell -*-
{-# LANGUAGE ForeignFunctionInterface #-}
{-# LANGUAGE MultiParamTypeClasses #-}

-- c2hs --cppopts='-U__BLOCKS__' --cppopts='-I.' --cppopts='-Isrc' --cppopts='-Ifuncsat' --cppopts='-Ifuncsat/src' Bitfunc_hs.chs

#include <bitfunc.h>
#include <bitfunc/bitvec.h>

module Bitfunc_hs
  ( bfInit
  , Literal
  , Bitvec(..)
  , bv_get_size
  )
   where

import C2HS
import Foreign.Ptr
import Foreign.ForeignPtr hiding (unsafeForeignPtrToPtr)
import Foreign.ForeignPtr.Unsafe(unsafeForeignPtrToPtr)
import Foreign.Marshal.Alloc
import System.IO.Unsafe(unsafePerformIO)
import Foreign.C
import Foreign.C.Types
import Data.Word
import Data.Array.IArray

data BFMan = BFMan

{#pointer *bfman as BFManPtr -> BFMan#}

instance Storable BFMan where
  sizeOf _ = {#sizeof bfman#}

newtype Literal = Literal { unLiteral :: CIntMax }

{#pointer *literal as Literals newtype#}

data Bitvec = Bitvec
  { literals :: Literals
  , size     :: Word32
  , capacity :: Word32
  , numHolds :: Word8
  }

{#pointer *bitvec as BitvecPtr -> Bitvec#}

bv_get_size :: BitvecPtr -> IO Word32
bv_get_size bv = {#get bitvec->size#} bv >>= return . fromIntegral

{#fun unsafe bfInit
  {fromIntegral `Int'}
  -> `BFManPtr' peek*#}



-- bv_getitem :: BitvecPtr -> Int -> IO Literal
-- bv_getitem bv i = do
--   lits <- {#get bitvec->data#} bv
--   l <- peekElemOff lits i
--   return $ Literal l
