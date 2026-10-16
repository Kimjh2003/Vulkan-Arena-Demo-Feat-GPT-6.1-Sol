package dev.demo.arena

import android.view.Choreographer
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/** UI-thread display pulses, coalesced to one pending render request. */
internal class FrameClock(private val display: DisplayProfileSource) : Choreographer.FrameCallback {
    private val choreographer=Choreographer.getInstance()
    private val pulse=Semaphore(0)
    private val pending=AtomicBoolean(false)
    @Volatile private var active=false

    fun start() {
        display.start()
        pulse.drainPermits()
        pending.set(false)
        active=true
        choreographer.postFrameCallback(this)
    }

    override fun doFrame(frameTimeNanos: Long) {
        if(!active) return
        // Old display pulses are discarded while the renderer is busy.
        signal()
        choreographer.postFrameCallback(this)
    }

    fun awaitFrame(): Boolean = try {
        if(pulse.tryAcquire(50,TimeUnit.MILLISECONDS)) {
            pending.set(false)
            active
        } else false
    } catch (_: InterruptedException) {
        Thread.currentThread().interrupt()
        false
    }

    fun stop() {
        active=false
        display.stop()
        choreographer.removeFrameCallback(this)
        signal() // At most one permit, including the lifecycle wakeup.
    }
    private fun signal() { if(pending.compareAndSet(false,true)) pulse.release() }
    fun refreshDisplay() = display.capture()
}
