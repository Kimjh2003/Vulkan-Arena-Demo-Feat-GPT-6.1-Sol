package dev.demo.arena

import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

/** UI produces one-shot actions, the render owner consumes them after prepare(). */
internal class RoundActions {
    private val dash=AtomicBoolean(false)
    private val upgrade=AtomicInteger(0)
    fun requestDash() { dash.set(true) }
    fun requestUpgrade(choice: Int) { upgrade.set(choice) }
    fun takeDash(): Boolean = dash.getAndSet(false)
    fun takeUpgrade(): Int = upgrade.getAndSet(0)
    fun reset() { dash.set(false);upgrade.set(0) }
}
