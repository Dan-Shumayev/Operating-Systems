//
// Created by dan-os on 12/06/2021.
//

#include "thread_context.h"
#include "job_context.h"


ThreadContext::ThreadContext(size_t tid, JobContext& jobContext)
: thread_id_(tid),
  pthreadThread_(),
  currentJobContext_(jobContext)
{
    if (pthread_create(&pthreadThread_, nullptr, _threadEntryPoint, static_cast<void *>(this)))
    {
        systemError("[[pthread_create]] failed.");
    }
}

void *ThreadContext::_threadEntryPoint(void *context)
{
    // Setup threadContext and jobContext
    auto threadContext = static_cast<ThreadContext*>(context);
    JobContext& currJobContext = threadContext->getJobContext();

    /** Map phase */
    threadContext->invokeMapPhase();

    /** Sort phase */
    threadContext->invokeSortPhase();

    /** Barrier - Let's wait here until all threads finish mapping and sorting */
    currJobContext.barrier();

    /** Shuffle phase */
    threadContext->invokeShufflePhase(); // TODO ensure inside this method that only thread-0 will execute Shuffle

    /** Barrier -  Let all threads wait here until thread 0 finishes shuffling */
    currJobContext.barrier();

    /** Reduce phase */
    threadContext->invokeReducePhase();

    return nullptr; // we must(as per-pthread_create) return something as we promise returning void-pointer, hence null
}

void ThreadContext::pthreadJoin()
{
    if (pthread_join(pthreadThread_, nullptr)) // TODO - pthread_join affects the thread's state?
                                                                    //TODO if not, make this method const
    {
        systemError("[[pthread_join]] failed.");
    }
}

void ThreadContext::invokeMapPhase()
{
    JobContext& currJobContext = currentJobContext_;
    size_t ix;
    while ((ix = currJobContext.lastThreadAtomicGetIncrement()) < currJobContext.getNumOfInputElems())
    {
        // distributing input elements among the thread workers
        auto elem = currJobContext.getInputVector()[ix];
        currJobContext.invokeClientMapRoutine(elem.first, elem.second, static_cast<void *>(this));
    }
    // TODO percentage, job state, etc. accordingly - to be continued...
}

void ThreadContext::invokeSortPhase()
{
    std::sort(intermediateVec_.begin(), intermediateVec_.end());
    // TODO - ensure we don't need anything else here
}

void ThreadContext::invokeShufflePhase()
{
    // TODO - implement logic
}

void ThreadContext::invokeReducePhase()
{
    // TODO - implement logic
}

void ThreadContext::pushIntermediateElem(IntermediatePair&& intermediatePair)
{
    intermediateVec_.emplace_back(std::move(intermediatePair)); // TODO - right use of std::move and rvalue?
}

void ThreadContext::pushOutputElem(OutputPair &&outputPair)
{
    currentJobContext_.updateOutputVector(std::move(outputPair)); // TODO - right use of std::move and rvalue?
}