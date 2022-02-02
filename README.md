# Azul - A darkchess-playing algorithm
This is the final project of the course "電腦對局理論(Theory of Computer Games: Fall 2021)" taught by professor 徐讚昇, I named it Azul (blue in portuguese) in reminiscence of the IBM chess system "Deep Blue", and also my Chinese name sound similar to blue in Chinese. The code is written in c++. The code show be paired with a match-making system to be fully functional. The official java match-making system which this code is tested with can be found [here](http://120.126.195.84/download.html), along with the manual.

Link to the course description : https://homepage.iis.sinica.edu.tw/~tshsu/tcg/2021/index.html

On the high level, the search algorithm is NegaScout with tricks. Most tricks revolve around reducing the search space in the early game, because the considering all moves including flips is about O(448), which is greater than go, and this greatly limits the search depth. 

`./boards` contains the match results pinning my bot against a simple baseline, which is a depth 3 negamax search with a basic evaulation function. The only exception is `./boards/COMPETITION(v7)`, which is the competition results against the other classmates. The logs could be visuallized with the match-making system.

## Change Logs
V1 NegaScout + Evaluation + Relavant Position Flipping

V2 Resolved Turn Counter Not Counting Error

V3 Added more robust time allocation mechanism to avoid timeout

V4 Resolved NegaScout Error

V5 Transposition Table

V6 Resolved NegaScout Error, fake move

V7 Added PV Path Heuristics

V8 Change depth i search algorithm : Can only flip before depth i -> Can only flip i times before 7 plys (Discarded because this was too slow)

## Competition
The competition is a swiss format across three days, the algorithm is allowed to change between opponents.
http://120.126.195.84/record.php?gametype=DarkChess&contest=NTU_CDC_Contest_20220113


Day1 - which bot : V7 
Results : 2W against R09944021 (9th place), 1W-1L against R09922115 (10th place), 2W against B10715058 (13th place)
Identified Issues : 
- Time comsumption too little (can think longer) (fixed in next version)

Day2 - which bot : v7turn30draw-75000
Results : 1W1L against R09922057 (2nd place), 2L against R10922066 (1st place), 1W1D against R10922077 (6th place)
Identified Issues : 
- Misclassification of board type : 帥仕 vs 將士, should be draw (bad move vs R10922077 1st round, move 118) (fixed in next version)

Day3 - which bot : v7turn30draw-75000_eatfirst
Results : 2D against R10922095 (3rd place), 1W1L against R10921071 (4th place), 1W1L against R10921118 (7th place)
Identified Issues : 
- Misclassification of board type : Winning too much may cause capturing a piece unfavorable during transition states (帥卒 ->no 帥卒)

Final Results : 5th place
