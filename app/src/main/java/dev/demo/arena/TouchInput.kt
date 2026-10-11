package dev.demo.arena

import android.view.MotionEvent
import android.view.View
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min

internal data class InputFrame(
    val moveX: Float = 0f, val moveZ: Float = 0f,
    val aimX: Float = 0f, val aimZ: Float = 0f,
    val shooting: Boolean = false
)

/** Pointer ownership lives on the UI thread; the render thread reads one immutable snapshot. */
internal class TouchInput {
    @Volatile var current = InputFrame()
        private set
    private var moveId = -1
    private var aimId = -1

    fun reset() { moveId = -1; aimId = -1; current = InputFrame() }

    fun onTouch(view: View, event: MotionEvent, settings: ControlSettings): Boolean {
        if (event.actionMasked == MotionEvent.ACTION_CANCEL) { reset(); return true }
        val index = event.actionIndex
        val id = event.getPointerId(index)
        if (event.actionMasked == MotionEvent.ACTION_DOWN || event.actionMasked == MotionEvent.ACTION_POINTER_DOWN) {
            val moveSide = if (settings.leftHanded) event.getX(index) > view.width / 2f
                           else event.getX(index) < view.width / 2f
            if (moveSide && moveId == -1) moveId = id
            if (!moveSide && settings.manualAim && aimId == -1) aimId = id
            if (event.actionMasked == MotionEvent.ACTION_DOWN) view.performClick()
        }
        val lifted = event.actionMasked == MotionEvent.ACTION_UP || event.actionMasked == MotionEvent.ACTION_POINTER_UP
        if (lifted) {
            if (moveId == id) moveId = -1
            if (aimId == id) aimId = -1
        }
        val moveCenter = if (settings.leftHanded) .82f else .18f
        val aimCenter = 1f - moveCenter
        val radius = (view.height * .14f).coerceAtLeast(1f)
        fun axis(pointer: Int, center: Float): Pair<Float, Float> {
            if (pointer == -1) return 0f to 0f
            val i = event.findPointerIndex(pointer)
            if (i < 0 || (lifted && i == index)) return 0f to 0f
            val dx = event.getX(i) - view.width * center
            val dz = event.getY(i) - view.height * .79f
            val distance = hypot(dx, dz)
            if (distance < radius * .16f) return 0f to 0f
            val magnitude = max(0f, min(1f, distance / radius) - .16f) / .84f
            return dx / distance * magnitude to dz / distance * magnitude
        }
        val move = axis(moveId, moveCenter)
        val aim = axis(aimId, aimCenter)
        current = InputFrame(move.first, move.second, aim.first, aim.second,
                             settings.manualAim && hypot(aim.first, aim.second) > .10f)
        return true
    }
}
