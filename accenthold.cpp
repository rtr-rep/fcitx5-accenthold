// Accent Hold — fcitx5 module.
//
// Tap a letter: it is typed as usual (on release).
// Hold a letter: after HOLD_DELAY the preedit starts rolling through its
// accented variants (é → è → ê → ë → e → é …) every ROLL_STEP; releasing the
// key commits whatever is showing. Shift/CapsLock give the capital variants.
// Esc while rolling commits the plain letter. Keys pressed with Ctrl/Alt/Super
// are never touched, so shortcuts keep working and autorepeating.

#include <fcitx-utils/event.h>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>

#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint64_t HOLD_DELAY_US = 400'000;
constexpr uint64_t ROLL_STEP_US = 450'000;

// First entry is the plain letter; the roll starts at index 1 and wraps back
// to the plain letter after the last variant. French first, then German.
// Punctuation keys are reached with Shift where needed (< > " $), which is
// fine since Shift is not treated as a shortcut modifier.
const std::unordered_map<fcitx::KeySym, std::vector<std::string>> &variants() {
    static const std::unordered_map<fcitx::KeySym, std::vector<std::string>> map = {
        {FcitxKey_a, {"a", "à", "â", "ä", "æ"}},
        {FcitxKey_c, {"c", "ç"}},
        {FcitxKey_e, {"e", "é", "è", "ê", "ë"}},
        {FcitxKey_i, {"i", "î", "ï"}},
        {FcitxKey_o, {"o", "ô", "ö", "œ"}},
        {FcitxKey_s, {"s", "ß"}},
        {FcitxKey_u, {"u", "ù", "û", "ü"}},
        {FcitxKey_y, {"y", "ÿ"}},
        {FcitxKey_A, {"A", "À", "Â", "Ä", "Æ"}},
        {FcitxKey_C, {"C", "Ç"}},
        {FcitxKey_E, {"E", "É", "È", "Ê", "Ë"}},
        {FcitxKey_I, {"I", "Î", "Ï"}},
        {FcitxKey_O, {"O", "Ô", "Ö", "Œ"}},
        {FcitxKey_S, {"S", "ẞ"}},
        {FcitxKey_U, {"U", "Ù", "Û", "Ü"}},
        {FcitxKey_Y, {"Y", "Ÿ"}},
        // Punctuation: guillemets, German and typographic quotes, currency.
        {FcitxKey_less, {"<", "«", "‹"}},
        {FcitxKey_greater, {">", "»", "›"}},
        {FcitxKey_apostrophe, {"'", "’", "‘"}},
        {FcitxKey_quotedbl, {"\"", "„", "“", "”"}},
        {FcitxKey_dollar, {"$", "€", "£"}},
    };
    return map;
}

} // namespace

namespace fcitx {

class AccentHold : public AddonInstance {
public:
    explicit AccentHold(Instance *instance) : instance_(instance) {
        handlers_.emplace_back(instance_->watchEvent(
            EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
            [this](Event &event) { onKey(static_cast<KeyEvent &>(event)); }));

        auto drop = [this](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            if (phase_ != Phase::Idle && ic_.get() == icEvent.inputContext()) {
                reset();
            }
        };
        for (auto type : {EventType::InputContextFocusOut,
                          EventType::InputContextReset,
                          EventType::InputContextSwitchInputMethod}) {
            handlers_.emplace_back(instance_->watchEvent(
                type, EventWatcherPhase::PreInputMethod, drop));
        }
    }

private:
    enum class Phase { Idle, Pending, Rolling };

    void onKey(KeyEvent &event) {
        auto *ic = event.inputContext();
        const Key &key = event.rawKey();

        if (phase_ != Phase::Idle) {
            if (ic_.get() != ic) {
                reset();
            } else if (key.code() == code_) {
                // Release commits; repeated presses (autorepeat) are swallowed.
                if (event.isRelease()) {
                    commit();
                }
                event.filterAndAccept();
                return;
            } else if (event.isRelease()) {
                return;
            } else if (phase_ == Phase::Rolling && key.sym() == FcitxKey_Escape) {
                index_ = 0;
                commit();
                event.filterAndAccept();
                return;
            } else {
                // Another key while held (fast typing rollover): flush the
                // held letter, then handle the new key normally below.
                commit();
            }
        }

        if (event.isRelease() ||
            key.states().testAny(KeyStates{KeyState::Ctrl, KeyState::Alt,
                                           KeyState::Super, KeyState::Super2,
                                           KeyState::Hyper})) {
            return;
        }
        auto it = variants().find(key.sym());
        if (it == variants().end()) {
            return;
        }

        phase_ = Phase::Pending;
        ic_ = ic->watch();
        code_ = key.code();
        cycle_ = &it->second;
        index_ = 0;
        timer_ = instance_->eventLoop().addTimeEvent(
            CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + HOLD_DELAY_US, 0,
            [this](EventSourceTime *source, uint64_t) {
                advance();
                source->setNextInterval(ROLL_STEP_US);
                source->setOneShot();
                return true;
            });
        event.filterAndAccept();
    }

    void advance() {
        if (phase_ == Phase::Pending) {
            phase_ = Phase::Rolling;
            index_ = 1;
        } else {
            index_ = (index_ + 1) % cycle_->size();
        }
        setPreedit(Text((*cycle_)[index_], TextFormatFlag::Underline));
    }

    void setPreedit(const Text &text) {
        auto *ic = ic_.get();
        if (!ic) {
            return;
        }
        if (ic->capabilityFlags().test(CapabilityFlag::Preedit)) {
            ic->inputPanel().setClientPreedit(text);
        } else {
            ic->inputPanel().setPreedit(text);
        }
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void commit() {
        auto *ic = ic_.get();
        std::string text = (*cycle_)[index_];
        bool hadPreedit = phase_ == Phase::Rolling;
        timer_.reset();
        phase_ = Phase::Idle;
        if (ic) {
            if (hadPreedit) {
                ic->inputPanel().setClientPreedit(Text());
                ic->inputPanel().setPreedit(Text());
                ic->updatePreedit();
                ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            }
            ic->commitString(text);
        }
        ic_.unwatch();
    }

    // Focus moved away or the context was reset: drop silently.
    void reset() {
        if (phase_ == Phase::Rolling) {
            setPreedit(Text());
        }
        timer_.reset();
        phase_ = Phase::Idle;
        ic_.unwatch();
    }

    Instance *instance_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>> handlers_;
    Phase phase_ = Phase::Idle;
    TrackableObjectReference<InputContext> ic_;
    int code_ = 0;
    const std::vector<std::string> *cycle_ = nullptr;
    size_t index_ = 0;
    std::unique_ptr<EventSourceTime> timer_;
};

class AccentHoldFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new AccentHold(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::AccentHoldFactory);
