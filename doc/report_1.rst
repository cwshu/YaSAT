SAT Solver milestone 1
----------------------

在 milestone 1, 我實作了以 backtrack 為主的基本版的 SAT solver.
其中比較特別的是 loop-based 的 backtracking 跟 2 literal watching 的 BCP.

不過, 還是先從 solver 的整體結構開始介紹.

1. before backtrack

   a. remove unit clause first: 

      如果一開始 CNF 中就有 unit clause, 那代表有 literal 已經被決定值了, 先做 implication 可以減少之後的 conflict.
      目前實作是先去掉所有 unit clause, 並把 unit clause 裡的 literal 都設定好數值.

      未來會考慮在加強兩個部份, 一是 unit clause 作 BCP, 二是設定好 literal 之後把所有 clauses 的該 literal 直接 remove 掉,
      降低 2 literal watching 的負擔.

      另外也會考慮此邏輯, 是否能整合到 heuristic search 加上 backtracking 的邏輯中, 也許不用額外特別實作.

   b. prepare for 2 literal watching:

      目前使用最簡單的作法, watch 每個 clause 的前兩個 literals.

2. backtrack

   由 ``DPLL_backtrack()`` 這個 function 實作.

   a. choose next literal: 目前沒有用 heuristic 的手法, 直接從第一個 literal 選到最後一個.
   b. BCP: 
   
      目前用 2 literal watching 來實作.

      這邊我設計上比較特別的是 literal watching 的更新. 每次更新都直接使用 ``update_literal_row()``, 把同個 clause 的 2 literal 一起檢查更新.
      這個設計的好處是可以確保 2 literal 上無論原本的值為何, 都可以 check 出 unit clause 跟 conflict clause.
      或者是, 當原本的 2 literal watching 要做該功能時, 經常需要 access another literal in clause 的運算. ``update_literal_row()`` 可以節省這個運算. 因為這個運算在我目前的程式中使用起來程式可讀性很低.

   c. conflict: 
   
      這邊採取很單純的 chronological backtracking, 也就是 conflict 之後直接倒退回上一個 decision state 的反向 state,
      如果上一個 decision state 的反向已經 conflict 過了, 那就再倒退回再上一個 decision state, recursively 進行相同操作.
      如果沒有 state 可以倒退了, 代表這組 CNF 是 unsatisfied.

      因為是 loop-based 的 backtracking, 所以會從 backtrack stack 上取得倒退回上個 state, 必須要倒退的所有 backtrack state [1].
      然後 rollback backtrack state, 設定 backtrack stack 為反向 state, 最後透過 ``continue`` 繼續 backtracking.

[1] backtrack stack 紀錄的 state: 這次 decision 被設定值的 literals 跟被 satisfied 的 clauses.
