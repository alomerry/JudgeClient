



[JudgeClient]()
=======

`Online Judge `  use docker and linux api to compile and run student's code


v1
---


* **make one client**
* run command:  
`docker run -p 80:80 -v /mnt/hgfs/webapp/problem_cases:/oj-home/problem_cases:ro -d mo/judge:v1`
`docker run -it /bin/bash -v /mnt/hgfs/webapp/problem_cases:/oj-home/problem_cases:ro -d mo/judge:v1`
* build command:  
`docker build -t mo/judge:v1 . `
---
next to do
```
1.finish judge serve
2.connect serve and client 
3.make docker file
4.fullfill judge client and serve
5.communicate with web page
```



