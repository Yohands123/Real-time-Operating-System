# Lab 3 Grading Report

## Table of Contents
[[_TOC_]]

## test0
Compilation & compatibility with evaluation framework ~ calls osKernelInit, k_mem_init, and printf.

### expected output
```
---- test0 ----
PASS: build successful
<serial timeout>
<end of test>
```
### your output
```
---- test0 ----
PASS: build successful
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test1
Cooperative multitasking still works — several tasks created using osCreateTask, which call osYield (after at least 1ms of execution time but before its deadline), will take turns in a Round Robin fashion (e.g. demo 1 of features post)

### expected output
```
---- test1 ----
1
2
3
1
2
3
1
2
3
1
2
3
1
2
3
<serial timeout>
<end of test>
```
### your output
```
---- test1 ----
1
2
3
1
2
3
1
2
3
1
2
3
1
2
3
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test2
If a task never yields, when its deadline is reached the kernel will pre-empt it (with the currently most urgent task). When the task is resumed later it will continue from where it was interrupted.

### expected output
```
---- test2 ----
A0 =0, B0 =0
A1 =[8000-45000], B1 =1
A2 =[16000-80000], B2 =2
A3 =[24000-120000], B3 =3
A4 =[32000-160000], B4 =4
<serial timeout>
<end of test>
```
### your output
```
---- test2 ----
A0 =0, B0 =0
A1 =24114, B1 =1
A2 =44950, B2 =2
A3 =65780, B3 =3
A4 =86604, B4 =4
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test3
Task stacks are created dynamically using k_mem_alloc, and are freed on osTaskExit. (If the heap is exhausted the task will not be created and will return RTX_ERR.)

### expected output
```
---- test3 ----
creating tasks with 0x1000 stack size until stack space is exhausted...
created task with tid-1, stack_high=0x2000cc30
created task with tid-2, stack_high=0x2000dc40
created task with tid-3, stack_high=0x2000ec50
...
PASS: osCreateTask failed (expected)

starting tasks...
task-1
task-2
stack_high: 0x2000dc40
exiting

task-3
creating task
stack_high of created task: 0x2000dc40
PASS: task stack freed & reused

task-4
<serial timeout>
<end of test>
```
### your output
```
---- test3 ----
creating tasks with 0x1000 stack size until stack space is exhausted...
created task with tid-1, stack_high=0x200076a8
created task with tid-2, stack_high=0x200086c0
created task with tid-3, stack_high=0x200096d8
created task with tid-4, stack_high=0x2000a6f0
created task with tid-5, stack_high=0x2000b708
created task with tid-6, stack_high=0x2000c720
created task with tid-7, stack_high=0x2000d738
created task with tid-8, stack_high=0x2000e750
created task with tid-9, stack_high=0x2000f768
created task with tid-10, stack_high=0x20010780
created task with tid-11, stack_high=0x20011798
created task with tid-12, stack_high=0x200127b0
created task with tid-13, stack_high=0x200137c8
PASS: osCreateTask failed (expected)

starting tasks...
task-1
task-2
task-3
task-4
task-5
task-6
task-7
task-8
task-9
task-10
task-11
task-12
task-13
stack_high of exiting task: 0x200086c0
task exiting...
creating new task
stack_high of newly created task: 0x200086c0
PASS: task stack freed & reused

task-14
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test4
Several tasks created using osCreateDeadlineTask with different deadlines will be started according to the order of "earliest deadline first"

### expected output
```
---- test4 ----
first
second
third
fourth
<serial timeout>
<end of test>
```
### your output
```
---- test4 ----
first
second
third
fourth
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test5
One task can run periodically via osPeriodYield (e.g. slide 1 of clarifications post)

### expected output
```
---- test5 ----
Using DWT for timing

A started 
Resume1 delta time =[3000-5000] us
Resume2 delta time =[3000-5000] us
Resume3 delta time =[3000-5000] us
Resume4 delta time =[3000-5000] us
Resume5 delta time =[3000-5000] us
Resume6 delta time =[3000-5000] us
Resume7 delta time =[3000-5000] us
Resume8 delta time =[3000-5000] us
Resume9 delta time =[3000-5000] us
Resume10 delta time =[3000-5000] us
<serial timeout>
<end of test>
```
### your output
```
---- test5 ----
Using DWT for timing

A started 
Resume1 delta time =4012 us
Resume2 delta time =4000 us
Resume3 delta time =4000 us
Resume4 delta time =4000 us
Resume5 delta time =4000 us
Resume6 delta time =4000 us
Resume7 delta time =4000 us
Resume8 delta time =4000 us
Resume9 delta time =4000 us
Resume10 delta time =4000 us
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test6
Multiple tasks can run periodically via osPeriodYield (e.g. demo 2 of features post)

### expected output
```
---- test6 ----
0, 0, 1
1, 1, 3
2, 1, 5
3, 1, 7
4, 2, 9
5, 2, 11
6, 2, 13
7, 3, 15
8, 3, 17
9, 3, 19
10, 4, 21
11, 4, 23
12, 4, 25
13, 5, 27
14, 5, 29
15, 5, 31
16, 6, 33
17, 6, 35
18, 6, 37
19, 7, 39
<serial timeout>
<end of test>
```
### your output
```
---- test6 ----
0, 0, 1
1, 1, 3
2, 1, 5
3, 1, 7
4, 2, 9
5, 2, 11
6, 2, 13
7, 3, 15
8, 3, 17
9, 3, 19
10, 4, 21
11, 4, 23
12, 4, 25
13, 5, 27
14, 5, 29
15, 5, 31
16, 6, 33
17, 6, 35
18, 6, 37
19, 7, 39
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test7
A simple task created with osCreateTask can use osSleep to run occasionally according to an arbitrary time schedule. 

### expected output
```
---- test7 ----
Using DWT for timing

Task started
osSleep 5 ms
Awake1 delta time =[4000-6000] us
osSleep 21 ms
Awake2 delta time =[20000-22000] us
osSleep 300 ms
Awake3 delta time =[299000-301000] us
osSleep 1 ms
Awake4 delta time =[100-2000] us

<serial timeout>
<end of test>
```
### your output
```
---- test7 ----
Using DWT for timing

Task started
osSleep 5 ms
Awake1 delta time =4325 us
osSleep 21 ms
Awake2 delta time =20849 us
osSleep 300 ms
Awake3 delta time =299655 us
osSleep 1 ms
Awake4 delta time =750 us

<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test8
Using osSetDeadline to make another task more urgent than the RUNNING task will trigger a context switch. And that task's deadline is permanently changed in future iterations. 

### expected output
```
---- test8 ----
A
B
A
B
set B deadline
B
A
B
B
A
B
B
<serial timeout>
<end of test>
```
### your output
```
---- test8 ----
A
B
A
B
set B deadline
B
A
B
B
A
B
B
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test9
Robustness -- test #6 (demo 2 of features post) runs continuously without glitches, for 500 iterations

### expected output
```
---- test9 ----
0, 0
1, 1
2, 1
3, 1
4, 2
5, 2
6, 2
7, 3
8, 3
9, 3
10, 4
11, 4
12, 4
13, 5
14, 5
15, 5
...
493, 165
494, 165
495, 165
496, 166
497, 166
498, 166
499, 167
<serial timeout>
<end of test>
```
### your output
```
---- test9 ----
0, 0
1, 1
2, 1
3, 1
4, 2
5, 2
6, 2
7, 3
8, 3
9, 3
10, 4
11, 4
12, 4
13, 5
14, 5
15, 5
16, 6
17, 6
18, 6
19, 7
20, 7
21, 7
22, 8
23, 8
24, 8
25, 9
26, 9
27, 9
28, 10
29, 10
30, 10
31, 11
32, 11
33, 11
34, 12
35, 12
36, 12
37, 13
38, 13
39, 13
40, 14
41, 14
42, 14
43, 15
44, 15
45, 15
46, 16
47, 16
48, 16
49, 17
50, 17
51, 17
52, 18
53, 18
54, 18
55, 19
56, 19
57, 19
58, 20
59, 20
60, 20
61, 21
62, 21
63, 21
64, 22
65, 22
66, 22
67, 23
68, 23
69, 23
70, 24
71, 24
72, 24
73, 25
74, 25
75, 25
76, 26
77, 26
78, 26
79, 27
80, 27
81, 27
82, 28
83, 28
84, 28
85, 29
86, 29
87, 29
88, 30
89, 30
90, 30
91, 31
92, 31
93, 31
94, 32
95, 32
96, 32
97, 33
98, 33
99, 33
100, 34
101, 34
102, 34
103, 35
104, 35
105, 35
106, 36
107, 36
108, 36
109, 37
110, 37
111, 37
112, 38
113, 38
114, 38
115, 39
116, 39
117, 39
118, 40
119, 40
120, 40
121, 41
122, 41
123, 41
124, 42
125, 42
126, 42
127, 43
128, 43
129, 43
130, 44
131, 44
132, 44
133, 45
134, 45
135, 45
136, 46
137, 46
138, 46
139, 47
140, 47
141, 47
142, 48
143, 48
144, 48
145, 49
146, 49
147, 49
148, 50
149, 50
150, 50
151, 51
152, 51
153, 51
154, 52
155, 52
156, 52
157, 53
158, 53
159, 53
160, 54
161, 54
162, 54
163, 55
164, 55
165, 55
166, 56
167, 56
168, 56
169, 57
170, 57
171, 57
172, 58
173, 58
174, 58
175, 59
176, 59
177, 59
178, 60
179, 60
180, 60
181, 61
182, 61
183, 61
184, 62
185, 62
186, 62
187, 63
188, 63
189, 63
190, 64
191, 64
192, 64
193, 65
194, 65
195, 65
196, 66
197, 66
198, 66
199, 67
200, 67
201, 67
202, 68
203, 68
204, 68
205, 69
206, 69
207, 69
208, 70
209, 70
210, 70
211, 71
212, 71
213, 71
214, 72
215, 72
216, 72
217, 73
218, 73
219, 73
220, 74
221, 74
222, 74
223, 75
224, 75
225, 75
226, 76
227, 76
228, 76
229, 77
230, 77
231, 77
232, 78
233, 78
234, 78
235, 79
236, 79
237, 79
238, 80
239, 80
240, 80
241, 81
242, 81
243, 81
244, 82
245, 82
246, 82
247, 83
248, 83
249, 83
250, 84
251, 84
252, 84
253, 85
254, 85
255, 85
256, 86
257, 86
258, 86
259, 87
260, 87
261, 87
262, 88
263, 88
264, 88
265, 89
266, 89
267, 89
268, 90
269, 90
270, 90
271, 91
272, 91
273, 91
274, 92
275, 92
276, 92
277, 93
278, 93
279, 93
280, 94
281, 94
282, 94
283, 95
284, 95
285, 95
286, 96
287, 96
288, 96
289, 97
290, 97
291, 97
292, 98
293, 98
294, 98
295, 99
296, 99
297, 99
298, 100
299, 100
300, 100
301, 101
302, 101
303, 101
304, 102
305, 102
306, 102
307, 103
308, 103
309, 103
310, 104
311, 104
312, 104
313, 105
314, 105
315, 105
316, 106
317, 106
318, 106
319, 107
320, 107
321, 107
322, 108
323, 108
324, 108
325, 109
326, 109
327, 109
328, 110
329, 110
330, 110
331, 111
332, 111
333, 111
334, 112
335, 112
336, 112
337, 113
338, 113
339, 113
340, 114
341, 114
342, 114
343, 115
344, 115
345, 115
346, 116
347, 116
348, 116
349, 117
350, 117
351, 117
352, 118
353, 118
354, 118
355, 119
356, 119
357, 119
358, 120
359, 120
360, 120
361, 121
362, 121
363, 121
364, 122
365, 122
366, 122
367, 123
368, 123
369, 123
370, 124
371, 124
372, 124
373, 125
374, 125
375, 125
376, 126
377, 126
378, 126
379, 127
380, 127
381, 127
382, 128
383, 128
384, 128
385, 129
386, 129
387, 129
388, 130
389, 130
390, 130
391, 131
392, 131
393, 131
394, 132
395, 132
396, 132
397, 133
398, 133
399, 133
400, 134
401, 134
402, 134
403, 135
404, 135
405, 135
406, 136
407, 136
408, 136
409, 137
410, 137
411, 137
412, 138
413, 138
414, 138
415, 139
416, 139
417, 139
418, 140
419, 140
420, 140
421, 141
422, 141
423, 141
424, 142
425, 142
426, 142
427, 143
428, 143
429, 143
430, 144
431, 144
432, 144
433, 145
434, 145
435, 145
436, 146
437, 146
438, 146
439, 147
440, 147
441, 147
442, 148
443, 148
444, 148
445, 149
446, 149
447, 149
448, 150
449, 150
450, 150
451, 151
452, 151
453, 151
454, 152
455, 152
456, 152
457, 153
458, 153
459, 153
460, 154
461, 154
462, 154
463, 155
464, 155
465, 155
466, 156
467, 156
468, 156
469, 157
470, 157
471, 157
472, 158
473, 158
474, 158
475, 159
476, 159
477, 159
478, 160
479, 160
480, 160
481, 161
482, 161
483, 161
484, 162
485, 162
486, 162
487, 163
488, 163
489, 163
490, 164
491, 164
492, 164
493, 165
494, 165
495, 165
496, 166
497, 166
498, 166
499, 167
<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## test10
Test10 Timing accuracy -- two tasks both execute on time within +-0.5ms accuracy (e.g. slide 2 of clarifications post)

### expected output
```
---- test10 ----
Using DWT for timing

measured execution time of Task B: 7209 us

Timestamps of Task A (deadline 4ms, execution time ~0.01ms) 
A start1 =[0+-500] us
A start2 =[4000+-500] us
A start3 =[8000+-500] us
A start4 =[12000+-500] us
A start5 =[16000+-500] us
A start6 =[20000+-500] us
A start7 =[24000+-500] us
A start8 =[28000+-500] us
A start9 =[32000+-500] us
A start10 =[36000+-500] us

Timestamps of Task B (deadline 11ms, execution time ~7.2ms): 
B start1 =[10+-500] us, B end1 =[7220+-500] us
B start2 =[11000+-500] us, B end2 =[18220+-500] us
B start3 =[22000+-500] us, B end3 =[29220+-500] us

<serial timeout>
<end of test>
```
### your output
```
---- test10 ----
Using DWT for timing

measured execution time of Task B: 7209 us

Timestamps of Task A (deadline 4ms, execution time ~0.01ms) 
A start1 =15 us
A start2 =4029 us
A start3 =8027 us
A start4 =12029 us
A start5 =16029 us
A start6 =20026 us
A start7 =24029 us
A start8 =28029 us
A start9 =32026 us
A start10 =36029 us

Timestamps of Task B (deadline 11ms, execution time ~7.2ms): 
B start1 =24 us, B end1 =7364 us
B start2 =11027 us, B end2 =18381 us
B start3 =22027 us, B end3 =29381 us

<serial timeout>
<end of test>

```
### your score: 1.00/1.00

## marking comments
{notes}



