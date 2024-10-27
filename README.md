WIP, things might change

This is based on the implementation in https://stackoverflow.com/questions/15752659/thread-pooling-in-c11 by https://stackoverflow.com/users/3818417/phd-ap-ece.
# ThreadPool class
Create `ThreadPool` object with
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
will start 10 threads the first time and 6 the second time `::start(...)` is called. The threads created by multiple instances of the `ThreadPool` class, however, can exceed the number of logical hardware processors.

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
where `f` is a lambda `[](void) -> void {}` (or `std::function<void(void)>`). You can queue as many jobs as you like and they will be started one by one whenever a thread is free.

## Await
All jobs that were queued before invocations of `::await()` will be executed.
```
ThreadPool pool(1);
pool.enq_job(f); // execution of f queued (and started shortly after)
pool.enq_job(g); // execution of g queued (waiting for f to finish to start executing)
pool.await();
// execution of both f and g finished
pool.stop();
```
Without `pool.await();` in the example above, `f` and `g` would probably not be executed at all, as the main calling thread is fast and stops the pool before calling the passed functions.

## Stop
You prevent unstarted jobs from the queue from being started with:
```
pool.stop();
```
Any running jobs will still be finished regularly.  

Make sure to call `::await()` first, if you want all jobs to be executed.

After stopping the `ThreadPool`, you can start it again, if you want unfinished jobs to be executed or if new jobs have been queued.

## Additional info
The destructor always calls `::stop()`.

You cannot catch exceptions thrown by the passed lambda or `std::function` (yet).