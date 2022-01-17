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
Results : 2W against 1W-1L against 2W against
Identified Issues : 
- Time comsumption too little (can think longer)

Day2 - which bot : v7turn30draw-75000
Results : 1W1L against, 2L against, 1W1D against
Identified Issues : 
- Misclassification of board type : 帥仕 vs 將士, should be draw (bad move vs R10922077 1st round, move 118) (fixed in next version)

Day3 - which bot : v7turn30draw-75000_eatfirst
Results : 1W1L against, 2D against, 1W1L against
Identified Issues : 
- Misclassification of board type : Winning too much may cause capturing a piece unfavorable during transition states (帥卒 ->no 帥卒)
