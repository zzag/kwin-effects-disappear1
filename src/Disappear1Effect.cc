/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Disappear1Effect.h"

// KConfigSkeleton
#include "disappear1config.h"

// Qt
#include <QtMath>

namespace {
const int Disappear1WindowRole = 0x22A98300;
const qreal s_fov = qTan(qDegreesToRadians(30.0));

inline qreal distanceToScale(qreal distance, qreal size)
{
    Q_ASSERT(size > 0);
    return 1.0 - qMin(1.0, 2.0 * distance * s_fov / size);
}
}

Disappear1Effect::Disappear1Effect()
{
    initConfig<Disappear1Config>();
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowAdded,
        this, &Disappear1Effect::markWindow);
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed,
        this, &Disappear1Effect::start);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &Disappear1Effect::stop);
}

Disappear1Effect::~Disappear1Effect()
{
}

void Disappear1Effect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags);

    Disappear1Config::self()->read();
    m_blacklist = Disappear1Config::blacklist().toSet();
    m_duration = animationTime(Disappear1Config::duration() > 0
            ? Disappear1Config::duration()
            : 160);
    m_opacity = Disappear1Config::opacity();
    m_shift = Disappear1Config::shift();
    m_distance = Disappear1Config::distance();
}

void Disappear1Effect::prePaintScreen(KWin::ScreenPrePaintData& data, int time)
{
    auto it = m_animations.begin();
    while (it != m_animations.end()) {
        Timeline& t = it.value();
        t.update(time);
        if (t.done()) {
            KWin::EffectWindow* w = it.key();
            w->unrefWindow();
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }

    if (!m_animations.isEmpty()) {
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }

    KWin::effects->prePaintScreen(data, time);
}

void Disappear1Effect::prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, int time)
{
    if (m_animations.contains(w)) {
        w->enablePainting(KWin::EffectWindow::PAINT_DISABLED_BY_DELETE);
        data.setTransformed();
    }

    KWin::effects->prePaintWindow(w, data, time);
}

void Disappear1Effect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    const auto it = m_animations.constFind(w);
    if (it != m_animations.cend()) {
        const qreal t = (*it).value();

        const qreal scale = distanceToScale(interpolate(0, m_distance, t), qMax(w->width(), w->height()));
        const qreal shift = interpolate(0, m_shift, t);

        data.setXScale(scale);
        data.setYScale(scale);
        data.setXTranslation(0.5 * (1 - scale) * w->width());
        data.setYTranslation(0.5 * (1 - scale) * w->height() + shift);
        data.multiplyOpacity(interpolate(1, m_opacity, t));
    }

    KWin::effects->paintWindow(w, mask, region, data);
}

void Disappear1Effect::postPaintScreen()
{
    if (!m_animations.isEmpty()) {
        KWin::effects->addRepaintFull();
    }

    KWin::effects->postPaintScreen();
}

bool Disappear1Effect::isActive() const
{
    return !m_animations.isEmpty();
}

bool Disappear1Effect::supported()
{
    return KWin::effects->isOpenGLCompositing()
        && KWin::effects->animationsSupported();
}

bool Disappear1Effect::shouldAnimate(const KWin::EffectWindow* w) const
{
    if (KWin::effects->activeFullScreenEffect()) {
        return false;
    }

    const auto* closeGrab = w->data(KWin::WindowClosedGrabRole).value<void*>();
    if (closeGrab != nullptr && closeGrab != this) {
        return false;
    }

    if (m_blacklist.contains(w->windowClass())) {
        return false;
    }

    if (w->data(Disappear1WindowRole).toBool()) {
        return true;
    }

    if (!w->isManaged()) {
        return false;
    }

    return w->isNormalWindow()
        || w->isDialog();
}

void Disappear1Effect::start(KWin::EffectWindow* w)
{
    if (!shouldAnimate(w)) {
        return;
    }

    // Tell other effects(like fade, for example) to ignore this window.
    w->setData(KWin::WindowClosedGrabRole, QVariant::fromValue(static_cast<void*>(this)));

    w->refWindow();

    Timeline& t = m_animations[w];
    t.setDuration(m_duration);
    t.setEasingCurve(QEasingCurve::OutCurve);
}

void Disappear1Effect::stop(KWin::EffectWindow* w)
{
    m_animations.remove(w);
}

void Disappear1Effect::markWindow(KWin::EffectWindow* w)
{
    if (!shouldAnimate(w)) {
        return;
    }
    w->setData(Disappear1WindowRole, true);
}
