# darkchess
Theory of Computer Games Homework Final - Dark Chess Bot

V1 NegaScout + Evaluation + Relavant Position Flipping

V2 Resolved Turn Counter Not Counting Error

V3 Added more robust time allocation mechanism to avoid timeout

V4 Resolved NegaScout Error

V5 Transposition Table

V6 Resolved NegaScout Error, fake move

V7 Added PV Path Heuristics

V8 Change depth i search algorithm : Can only flip before depth i -> Can only flip i times before 7 plys

## Competition
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
http://120.126.195.84/record.php?gametype=DarkChess&contest=NTU_CDC_Contest_20220113
