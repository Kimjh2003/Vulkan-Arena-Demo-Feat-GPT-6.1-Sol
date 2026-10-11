package dev.demo.arena

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.view.MotionEvent
import android.view.View
import android.util.TypedValue

internal class VirtualControls(context: Context, private val input: TouchInput,
                               private val settings: ControlSettings) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val state = input.current
        val moveCenter = if (settings.leftHanded) .82f else .18f
        fun stick(center: Float, dx: Float, dz: Float, label: String, tint: Int) {
            val x = width * center; val y = height * .79f; val r = height * .14f
            paint.style = Paint.Style.FILL; paint.color = Color.argb(65, 10, 24, 38)
            canvas.drawCircle(x, y, r, paint)
            paint.style = Paint.Style.STROKE; paint.strokeWidth = 2f * resources.displayMetrics.density
            paint.color = Color.argb(125, 130, 200, 230); canvas.drawCircle(x, y, r, paint)
            paint.style = Paint.Style.FILL; paint.color = tint
            canvas.drawCircle(x + dx*r*.72f, y + dz*r*.72f, r*.28f, paint)
            paint.color = Color.argb(200, 225, 240, 255); paint.textAlign = Paint.Align.CENTER
            paint.textSize = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 12f, resources.displayMetrics)
            canvas.drawText(label, x, y + r + paint.textSize*1.4f, paint)
        }
        stick(moveCenter, state.moveX, state.moveZ, "MOVE", Color.argb(190, 35, 205, 245))
        if (settings.manualAim)
            stick(1f-moveCenter, state.aimX, state.aimZ, "AIM / FIRE", Color.argb(190, 255, 180, 55))
    }
    override fun onTouchEvent(event: MotionEvent): Boolean {
        input.onTouch(this, event, settings)
        invalidate()
        return true
    }
    override fun performClick(): Boolean { super.performClick(); return true }
}
