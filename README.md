This is based on the implementation in https://stackoverflow.com/questions/15752659/thread-pooling-in-c11 by https://stackoverflow.com/users/3818417/phd-ap-ece.
# ThreadPool class
Create <code>ThreadPool</code> object with
```
ThreadPool pool;
```
and initialize threads with
```
size_t num_threads = 10;
pool.start(num_threads);
```
The number of active threads of one instance will never exceed the number of logical hardware processors. Assuming, the CPU has 16 threads, this
```
ThreadPool pool;
pool.start(10);
pool.start(10);
```
will start 10 threads the first time and 6 the second time <code>start(...)</code> is called. The threads created by multiple instances of the <code>ThreadPool</code> object, however, can exceed the number of logical hardware processors.

```
ThreadPool pool; pool.start(10);
// is equivalent to
// ThreadPool pool(10);
```

## Jobs
Jobs can be queued with
```
pool.enq_job(f);
```
where <code>f</code> is a lambda <code>\[\](void) -> void {}</code> (or <code>std::function\<void(void)\></code>). You can queue as many jobs as you like and they will be started whenever a thread is free.

## Stop
You prevent new jobs from being started with:
```
pool.stop();
```
Any runnings jobs will still be finished regularly.  

## Await
Of course, usually, you want all jobs to be executed. Use this:
```
pool.await();
pool.stop();
```