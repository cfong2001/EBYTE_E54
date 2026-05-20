"""
Shared utility functions for HLK-LD2450 radar data processing.
"""
import math

def s16_le(b0, b1):
    """
    Convert two bytes to a signed 16-bit integer using standard 2's complement.
    Used in some versions of the radar protocol.
    """
    v = b0 | (b1 << 8)
    return v - 0x10000 if v & 0x8000 else v

def ld2450_s16(b0, b1):
    """
    Convert little-endian signed int16 per LD2450 protocol spec.
    Protocol spec: highest bit 1=positive, 0=negative (inverted from standard).
    """
    v = b0 | (b1 << 8)
    if v & 0x8000:
        return v - 0x8000  # Positive: remove sign bit
    else:
        return -v if v != 0 else 0  # Negative: negate the value

def calculate_multi_anchor_stabilization(targets, state_dict, dt=0.1):
    """
    Perform multi-anchor stabilization and prediction for radar targets.

    Args:
        targets: List of dicts like {'id': i, 'active': bool, 'x': mm, 'y': mm, 'speed': cm/s}
        state_dict: Persistent dictionary mapping target IDs to their Alpha-Beta filter states
                    {'active': bool, 'is_anchor': bool, 'x': mm, 'y': mm, 'vel_x': mm/s, 'vel_y': mm/s}
        dt: Time delta between frames (seconds).

    Returns:
        stabilized_targets: List of dicts with the stabilized coordinates
                            {'id': i, 'active': bool, 'x': mm, 'y': mm, 'vel_x': mm/s, 'vel_y': mm/s}
    """

    # 1. Predict next state
    predicted = []
    for t in targets:
        tid = t['id']
        if tid not in state_dict:
            state_dict[tid] = {'active': False, 'is_anchor': False, 'x': t['x'], 'y': t['y'], 'vel_x': 0.0, 'vel_y': 0.0}

        st = state_dict[tid]
        if st['active']:
            px = st['x'] + st['vel_x'] * dt
            py = st['y'] + st['vel_y'] * dt
        else:
            px = t['x']
            py = t['y']
        predicted.append((px, py))

    # 2. Identify Anchors
    temp_anchors = []
    STATIC_SPEED_THRESHOLD = 30 # cm/s (15 * 2)
    for i, t in enumerate(targets):
        tid = t['id']
        st = state_dict[tid]
        if t['active'] and st['active']:
            if abs(t.get('speed', 0)) < STATIC_SPEED_THRESHOLD:
                temp_anchors.append(i)
                st['is_anchor'] = True
            else:
                st['is_anchor'] = False
        else:
            st['is_anchor'] = False

    # Dynamic Anchor Validation (Cross-Target Rigidity Rejection)
    anchors = []
    num_anchors = len(temp_anchors)
    if num_anchors == 3:
        err = [0.0, 0.0, 0.0]
        for i in range(3):
            a = temp_anchors[i]
            for j in range(i + 1, 3):
                b = temp_anchors[j]
                dQx = targets[a]['x'] - targets[b]['x']
                dQy = targets[a]['y'] - targets[b]['y']
                dist_q_sq = dQx*dQx + dQy*dQy

                dPx = predicted[a][0] - predicted[b][0]
                dPy = predicted[a][1] - predicted[b][1]
                dist_p_sq = dPx*dPx + dPy*dPy

                diff = abs(dist_q_sq - dist_p_sq) / (dist_p_sq + 1.0)
                err[i] += diff
                err[j] += diff
        # Find the minimum error. In a 3-point system where 1 point moves,
        # the two points that don't move relative to each other will share the lowest error component.
        # Find if there is an anomalous anchor:
        min_err = min(err)
        for i in range(3):
            # Allow up to 0.3 absolute error, or relatively close to the minimum error
            if err[i] < 0.3 or err[i] <= (min_err * 2.0 + 0.1):
                anchors.append(temp_anchors[i])
            else:
                state_dict[targets[temp_anchors[i]]['id']]['is_anchor'] = False

    elif num_anchors == 2:
        a = temp_anchors[0]
        b = temp_anchors[1]
        dQx = targets[a]['x'] - targets[b]['x']
        dQy = targets[a]['y'] - targets[b]['y']
        dPx = predicted[a][0] - predicted[b][0]
        dPy = predicted[a][1] - predicted[b][1]

        diff = abs((dQx*dQx + dQy*dQy) - (dPx*dPx + dPy*dPy)) / ((dPx*dPx + dPy*dPy) + 1.0)
        if diff < 0.2:
            anchors = temp_anchors
        else:
            state_dict[targets[a]['id']]['is_anchor'] = False
            state_dict[targets[b]['id']]['is_anchor'] = False

    elif num_anchors == 1:
        anchors = temp_anchors

    # 3. Calculate Global Rigid Transformation (Kabsch algorithm in 2D)
    Cp_x, Cp_y, Cq_x, Cq_y = 0.0, 0.0, 0.0, 0.0
    cos_t, sin_t = 1.0, 0.0
    num_anchors = len(anchors)

    if num_anchors > 0:
        for i in anchors:
            Cp_x += predicted[i][0]
            Cp_y += predicted[i][1]
            Cq_x += targets[i]['x']
            Cq_y += targets[i]['y']
        Cp_x /= num_anchors
        Cp_y /= num_anchors
        Cq_x /= num_anchors
        Cq_y /= num_anchors

        if num_anchors >= 2:
            S, C = 0.0, 0.0
            for i in anchors:
                vPx = predicted[i][0] - Cp_x
                vPy = predicted[i][1] - Cp_y
                vQx = targets[i]['x'] - Cq_x
                vQy = targets[i]['y'] - Cq_y
                S += (vPx * vQy - vPy * vQx)
                C += (vPx * vQx + vPy * vQy)
            M = math.sqrt(C * C + S * S)
            if M > 1.0:
                cos_t = C / M
                sin_t = S / M

    # 4. Stabilize and update Alpha-Beta-Gamma filters
    base_alpha, base_beta, base_gamma = 0.6, 0.12, 0.05
    stabilized = []

    for i, t in enumerate(targets):
        tid = t['id']
        st = state_dict[tid]

        if 'acc_x' not in st:
            st['acc_x'] = 0.0
            st['acc_y'] = 0.0

        res = {'id': tid, 'active': t['active'], 'x': t['x'], 'y': t['y'], 'vel_x': 0.0, 'vel_y': 0.0, 'acc_x': 0.0, 'acc_y': 0.0}

        if t['active']:
            qx, qy = t['x'], t['y']
            stab_x, stab_y = qx, qy

            if num_anchors > 0:
                vQx = qx - Cq_x
                vQy = qy - Cq_y
                vQx_rot = vQx * cos_t + vQy * sin_t
                vQy_rot = -vQx * sin_t + vQy * cos_t
                stab_x = Cp_x + vQx_rot
                stab_y = Cp_y + vQy_rot

            res['x'] = stab_x
            res['y'] = stab_y

            if not st['active']:
                st['active'] = True
                st['x'] = stab_x
                st['y'] = stab_y
                st['vel_x'] = 0.0
                st['vel_y'] = 0.0
                st['acc_x'] = 0.0
                st['acc_y'] = 0.0
            else:
                alpha, beta, gamma = base_alpha, base_beta, base_gamma
                resid_x = stab_x - predicted[i][0]
                resid_y = stab_y - predicted[i][1]
                dist_sq = resid_x * resid_x + resid_y * resid_y

                if st['is_anchor']:
                    if dist_sq > 10000.0:
                        alpha *= 0.2
                        beta *= 0.1
                        gamma *= 0.1
                    elif dist_sq < 400.0:
                        alpha = min(1.0, alpha * 1.5)
                        beta *= 1.5
                        gamma *= 1.5
                else:
                    alpha = min(1.0, alpha * 2.0)
                    beta = alpha * 0.2
                    gamma = alpha * 0.05

                st['x'] = predicted[i][0] + alpha * resid_x
                st['y'] = predicted[i][1] + alpha * resid_y
                st['vel_x'] = st['vel_x'] + (beta * resid_x / dt)
                st['vel_y'] = st['vel_y'] + (beta * resid_y / dt)

                dt_sq_half = (dt * dt) * 0.5
                st['acc_x'] = st['acc_x'] + (gamma * resid_x / dt_sq_half)
                st['acc_y'] = st['acc_y'] + (gamma * resid_y / dt_sq_half)

                # Constrain acceleration
                max_acc = 5000.0
                st['acc_x'] = max(-max_acc, min(max_acc, st['acc_x']))
                st['acc_y'] = max(-max_acc, min(max_acc, st['acc_y']))

            res['vel_x'] = st['vel_x']
            res['vel_y'] = st['vel_y']
            res['acc_x'] = st['acc_x']
            res['acc_y'] = st['acc_y']
        else:
            st['active'] = False
            st['is_anchor'] = False

        stabilized.append(res)

    return stabilized
