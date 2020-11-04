module Main where

import qualified Bitfunc_hs as Bitfunc
import Text.Printf

main = do
  bf <- Bitfunc.bfInit 0

  return 0
  -- v0 <- Bitfunc.vInit bf 32
  -- r <- Bitfunc.solve bf
  -- cx <- Bitfunc.vFromCounterExample bf v0
  -- printf "%s\n" (show cx)
