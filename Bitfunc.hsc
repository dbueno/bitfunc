{-# LANGUAGE MultiParamTypeClasses #-}

module Bitfunc where

import Foreign.Ptr
import Foreign.ForeignPtr hiding (unsafeForeignPtrToPtr)
import Foreign.ForeignPtr.Unsafe(unsafeForeignPtrToPtr)
import Foreign.Marshal.Alloc
import System.IO.Unsafe(unsafePerformIO)
import Foreign.C
import Foreign.C.Types
import Data.Word
import Data.Array.IArray

newtype BFResult = BFResult { unBFResult :: CInt }
  deriving (Eq, Ord)

#include <bitfunc/types.h>

#{enum BFResult, BFResult
  , bf_unknown = BF_UNKNOWN
  , bf_sat     = BF_SAT
  , bf_unsat   = BF_UNSAT
  }
instance Show BFResult where
  show x | x == bf_unknown = "unknown"
         | x == bf_sat = "SAT"
         | x == bf_unsat = "UNSAT"
         | True = error "hi"

newtype BFStatus = BFStatus { unBFStatus :: CInt }
#{enum BFStatus, BFStatus
  , status_must_be_true = STATUS_MUST_BE_TRUE
  , status_must_be_false = STATUS_MUST_BE_FALSE
  , status_true_or_false = STATUS_TRUE_OR_FALSE
  , status_not_true_nor_false = STATUS_NOT_TRUE_NOR_FALSE
  , status_unknown = STATUS_UNKNOWN
  }

newtype Literal = Literal { unLiteral :: CIntMax }

type BitvecObj = ()
data Bitvec i e = Bitvec { unBitvec :: !(ForeignPtr BitvecObj) }
bitvecPtr = unsafeForeignPtrToPtr . unBitvec

instance Storable Foo where
    sizeOf    _ = #{size bitvec}
    alignment _ = alignment (undefined :: CInt)

    -- poke p foo  = do
    --   #{poke foo, i1} p $ i1 foo
    --   #{poke foo, i2} p $ i2 foo

    peek p = return Bitvec
              `ap` (#{peek bitvec, i1} p)
              `ap` (#{peek bitvec, i2} p)

instance IArray Bitvec Literal where

type BFManObj = ()
data BFMan = BFMan { unBFMan :: !(ForeignPtr BFManObj) }
             deriving (Eq, Ord, Show)
bfmanPtr = unsafeForeignPtrToPtr . unBFMan


foreign import ccall unsafe "bitfunc.h bfInit"
  c_bfInit  :: CInt
            -> IO (Ptr BFManObj)
foreign import ccall unsafe "bitfunc.h bfDestroy"
  c_bfDestroy  :: Ptr BFManObj
               -> IO ()
foreign import ccall "bitfunc.h &bfDestroy"
  c_bfDestroy_ptr  :: FunPtr (Ptr BFManObj -> IO ())

foreign import ccall unsafe "bitfunc.h bfPushAssumption"
  c_bfPushAssumption :: Ptr BFManObj -> Literal -> IO ()

foreign import ccall unsafe "bitfunc.h bfSolve"
  c_bfSolve :: Ptr BFManObj -> IO BFResult

foreign import ccall unsafe "bitvec.h bfvInit"
  c_bfvInit :: Ptr BFManObj
            -> Word32
            -> IO (Ptr BitvecObj)

foreign import ccall unsafe "bitvec.h bfvAlloc"
  c_bfvAlloc :: Word32 -> IO (Ptr BitvecObj)
foreign import ccall unsafe "bitvec.h bfvHold"
  c_bfvHold :: Ptr BitvecObj -> IO (Ptr BitvecObj)
foreign import ccall unsafe "bitvec.h bfvRelease"
  c_bfvRelease :: Ptr BitvecObj -> IO (Ptr BitvecObj)
foreign import ccall unsafe "bitvec.h &bfvReleaseConsume"
  c_bfvReleaseConsume :: FunPtr (Ptr BitvecObj -> IO ())

wrapBV :: Ptr BitvecObj -> IO Bitvec
wrapBV bv = do
  c_bfvHold bv
  bv_fp <- newForeignPtr c_bfvReleaseConsume bv
  return $ Bitvec bv_fp



foreign import ccall unsafe "bitvec.h &bfvDestroy"
  c_bfvDestroy_ptr :: FunPtr (Ptr BitvecObj -> IO ())

foreign import ccall unsafe "bitfunc.h bflFromCounterExample"
  c_bflFromCounterExample :: Ptr BFManObj
                          -> Literal
                          -> IO Literal

foreign import ccall unsafe "bitfunc.h bfvFromCounterExample"
  c_bfvFromCounterExample :: Ptr BFManObj
                          -> Ptr BitvecObj
                          -> IO (Ptr BitvecObj)
------------------------------------------------------------------------------
-- Haskell API
------------------------------------------------------------------------------


init :: IO BFMan
init = do
  bfp <- c_bfInit 0
  fp <- newForeignPtr c_bfDestroy_ptr bfp
  return $ BFMan fp

vInit :: BFMan -> Word32 -> IO Bitvec
vInit bf sz = do
  bv_ptr <- c_bfvInit (bfmanPtr bf) sz
  wrapBV bv_ptr

lFromCounterExample :: BFMan -> Literal -> IO Literal
lFromCounterExample bf =
  c_bflFromCounterExample (bfmanPtr bf)

vFromCounterExample :: BFMan -> Bitvec -> IO Bitvec
vFromCounterExample bf bv =
  wrapBV =<< c_bfvFromCounterExample (bfmanPtr bf) (bitvecPtr bv)
  

solve :: BFMan -> IO BFResult
solve = c_bfSolve . bfmanPtr
