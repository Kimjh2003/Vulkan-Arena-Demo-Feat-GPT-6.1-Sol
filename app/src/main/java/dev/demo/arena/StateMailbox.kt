package dev.demo.arena

import android.os.Handler
import android.os.Looper
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference

/** Latest immutable HUD state, one reusable dispatch runnable, no stale callback backlog. */
internal class StateMailbox(private val consume: (HudState)->Unit) : Runnable {
    private val handler=Handler(Looper.getMainLooper())
    private val latest=AtomicReference<HudState?>(null)
    private val queued=AtomicBoolean(false)
    @Volatile private var closed=false
    fun offer(state: HudState) {
        if(closed) return
        latest.set(state)
        schedule()
    }
    private fun schedule() {
        if(!closed && queued.compareAndSet(false,true)) handler.post(this)
    }
    override fun run() {
        val state=latest.getAndSet(null)
        queued.set(false)
        if(!closed && state!=null) consume(state)
        if(latest.get()!=null) schedule()
    }
    fun close() {
        closed=true;latest.set(null);handler.removeCallbacks(this);queued.set(false)
    }
}
